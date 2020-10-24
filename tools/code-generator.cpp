/***************************************************************************
 *   Copyright (C) 2018-2020 by Robert Middleton                           *
 *   robert.middleton@rm5248.com                                           *
 *                                                                         *
 *   This file is part of the dbus-cxx library.                            *
 *                                                                         *
 *   The dbus-cxx library is free software; you can redistribute it and/or *
 *   modify it under the terms of the GNU General Public License           *
 *   version 3 as published by the Free Software Foundation.               *
 *                                                                         *
 *   The dbus-cxx library is distributed in the hope that it will be       *
 *   useful, but WITHOUT ANY WARRANTY; without even the implied warranty   *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU   *
 *   General Public License for more details.                              *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this software. If not see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/
#include <iostream>
#include <fstream>
#include <dbus-cxx.h>
#include <string>

#include "code-generator.h"

using DBus::CodeGenerator;

CodeGenerator::CodeGenerator(){
    m_parser = XML_ParserCreate(NULL);
    XML_SetElementHandler( m_parser, CodeGenerator::start_element_handler, CodeGenerator::end_element_handler );
    XML_SetUserData( m_parser, this );
}

CodeGenerator::~CodeGenerator(){
    XML_ParserFree( m_parser );
}

void CodeGenerator::setXML( const std::string& xmlDocument ){
    m_xmlData = xmlDocument;
}

bool CodeGenerator::parse(){
    if( XML_Parse( m_parser, m_xmlData.c_str(), m_xmlData.size(), 1 ) == XML_STATUS_ERROR ){
        std::cerr << "XML Error: " << XML_ErrorString(XML_GetErrorCode(m_parser)) << std::endl;
        std::cerr << "Line:" << XML_GetCurrentLineNumber(m_parser) << std::endl;
        return false;
    }

    return true;
}

void CodeGenerator::start_element( std::string tagName, std::map<std::string,std::string> tagAttrs ){
    std::string parentElement = m_tagStack.empty() ? "" : m_tagStack.top();
    m_tagStack.push( tagName );

    if( tagName.compare( "node" ) == 0 ){
        handle_node_tag( tagAttrs );
    }else if( tagName.compare( "interface" ) == 0 ){
        if( tagAttrs.find( "name" ) == tagAttrs.end() ){
            std::cerr << "WARNING: No name for interface found" << std::endl;
            return;
        }
        m_currentInterface = tagAttrs[ "name" ];
    }else if( tagName.compare( "method" ) == 0 ){
        handle_method_tag( tagAttrs );
    }else if( tagName.compare( "signal" ) == 0 ){
        handle_signal_tag( tagAttrs );
    }else if( tagName.compare( "arg" ) == 0 ){
        handle_arg_tag( parentElement, tagAttrs );
    }else if( tagName.compare( "property" ) == 0 ){
        handle_property_tag( tagAttrs );
    }
}

void CodeGenerator::end_element( std::string tagName ){
    m_tagStack.pop();

    if( tagName == "method" ){
        /* Add a method to the class, along with its implementation
         * to actually call over the bus
         */
        cppgenerate::MemberVariable memberVar;
        std::vector<cppgenerate::Argument> args = m_currentProxyMethod.arguments();
        std::string dbusSig;
        /* method proxy type = template args */
        std::string methodProxyType;
        /* methodArguments = arguments to proxy method */
        std::string methodArguments;
        bool argumentComma = false;
        int argNum = 0;
        std::string block;

        methodProxyType += "<" + m_currentProxyMethod.returnType();
        methodProxyType += "(";
        for( cppgenerate::Argument arg : args ){
            if( argumentComma ){
                methodArguments += ",";
                methodProxyType += ",";
            }

            methodProxyType += arg.type();
            methodArguments += arg.name();
            argumentComma = true;
        }
        methodProxyType += ")";
        methodProxyType += ">";

        memberVar.setAccessModifier( cppgenerate::AccessModifier::PROTECTED )
                 .setName( "m_method_" + m_currentProxyMethod.name() )
                 .setType( "std::shared_ptr<DBus::MethodProxy" + methodProxyType + "> " );

        if( m_currentProxyMethod.returnType() != "void" ){
            block = "return ";
        }
        block += "(*" + memberVar.name() + ")(" +  methodArguments + ");";
        m_currentProxyMethod.addCode( cppgenerate::CodeBlock::create()
            .addLine( block ) );

        m_proxyClasses.data()[ m_proxyClasses.size() - 1 ]
            .addMethod( m_currentProxyMethod )
            .addMemberVariable( memberVar );

        m_currentProxyConstructor.addCode( cppgenerate::CodeBlock::create()
            .addLine( "m_method_" + m_currentProxyMethod.name() + 
                      " = this->create_method" + methodProxyType + 
                      "(\"" + m_currentInterface + "\",\"" + m_currentProxyMethod.name() + "\");" ) );

        /* Add our new virtual method that needs to be implemented to our adaptee */
        m_adapteeClasses.data()[ m_adapteeClasses.size() - 1 ]
            .addMethod( m_currentAdapteeMethod );

        /* Add our internal construction of the adaptee method */
        m_currentAdapterConstructor.addCode( cppgenerate::CodeBlock::create()
            .addLine( "temp_method = this->create_method" + methodProxyType + 
                      "( \"" + m_currentInterface + "\", \"" + 
                      m_currentAdapteeMethod.name() + "\"," +
                      "sigc::mem_fun( *adaptee, &" + m_adapteeClasses.data()[ m_adapteeClasses.size() - 1 ].getName() +
                      "::" + m_currentAdapteeMethod.name() + " ) );" ) );
        m_currentAdapterConstructor.addCode( cppgenerate::CodeBlock::create()
            .addLine( "temp_method->set_arg_name( 0, \"" + m_returnName + "\" );" ) );
        for( cppgenerate::Argument arg : args ){
            argNum++;
            m_currentAdapterConstructor.addCode( cppgenerate::CodeBlock::create()
                .addLine( "temp_method->set_arg_name( " + std::to_string( argNum ) + ", \"" + arg.name() + "\" );" ) );
        }
    }else if( tagName == "node" ){
        m_proxyClasses.data()[ m_proxyClasses.size() - 1 ]
            .addConstructor( m_currentProxyConstructor );

        m_adapterClasses.data()[ m_adapterClasses.size() - 1 ]
            .addConstructor( m_currentAdapterConstructor );
    }else if( tagName == "signal" ){
        cppgenerate::MemberVariable adapterMemberVar;
        cppgenerate::MemberVariable proxyMemberVar;
        cppgenerate::Method getSignalMethod;
        cppgenerate::Method emitSignalMethod;
        cppgenerate::Method proxy_getSignalMethod;
        std::vector<cppgenerate::Argument> args = m_currentSignal.arguments();
        std::string templateType;
        bool argumentComma = false;
        std::string signalEmitCode;

        templateType += "<void(";
        signalEmitCode = "(*m_signal_" + m_currentSignal.name() + ")(";
        for( cppgenerate::Argument arg : args ){
            emitSignalMethod.addArgument( arg );

            if( argumentComma ){
                templateType += ",";
                signalEmitCode += ",";
            }

            templateType += arg.type();
            signalEmitCode += arg.name();
            argumentComma = true;
        }
        templateType += ")>";
        signalEmitCode += ");";

        proxyMemberVar.setAccessModifier( cppgenerate::AccessModifier::PROTECTED )
                 .setName( "m_signalproxy_" + m_currentSignal.name() )
                 .setType( "std::shared_ptr<DBus::signal_proxy" + templateType + ">" );

        adapterMemberVar.setAccessModifier( cppgenerate::AccessModifier::PROTECTED )
                 .setName( "m_signal_" + m_currentSignal.name() )
                 .setType( "std::shared_ptr<DBus::signal" + templateType + ">" );

        proxy_getSignalMethod = cppgenerate::Method::create()
            .setAccessModifier( cppgenerate::AccessModifier::PUBLIC )
            .setReturnType( "std::shared_ptr<DBus::signal_proxy" + templateType + ">" )
            .setName( "signal_" + m_currentSignal.name() )
            .setCode( cppgenerate::CodeBlock::create().addLine( "return m_signalproxy_" + m_currentSignal.name() + ";" ) );

        getSignalMethod = cppgenerate::Method::create()
            .setAccessModifier( cppgenerate::AccessModifier::PUBLIC )
            .setReturnType( "std::shared_ptr<DBus::signal" + templateType + ">" )
            .setName( "signal_" + m_currentSignal.name() )
            .setCode( cppgenerate::CodeBlock::create().addLine( "return m_signal_" + m_currentSignal.name() + ";" ) );

        emitSignalMethod.setName( m_currentSignal.name() )
            .setAccessModifier( cppgenerate::AccessModifier::PUBLIC )
            .setCode( cppgenerate::CodeBlock::create().addLine( signalEmitCode ) );
        
        m_adapterClasses.data()[ m_adapterClasses.size() - 1 ]
            .addMemberVariable( adapterMemberVar )
            .addMethod( getSignalMethod )
            .addMethod( emitSignalMethod );

        m_proxyClasses.data()[ m_proxyClasses.size() - 1 ]
            .addMemberVariable( proxyMemberVar )
            .addMethod( proxy_getSignalMethod );

        /* Add our internal construction of the adapter signal */
        m_currentAdapterConstructor.addCode( cppgenerate::CodeBlock::create()
            .addLine( adapterMemberVar.name() + 
                      " = this->create_signal" + templateType + 
                      "( \"" + m_currentInterface + "\", \"" + 
                      m_currentSignal.name() + "\" );" ) );

        m_currentProxyConstructor.addCode( cppgenerate::CodeBlock::create()
            .addLine( proxyMemberVar.name() + 
                      " = this->create_signal" + templateType + 
                      "( \"" + m_currentInterface + "\", \"" + 
                      m_currentSignal.name() + "\", signalCallingThread );" ) );

    }
}

void CodeGenerator::start_element_handler(void* user_data, const XML_Char* name, const XML_Char** attrs ){
    CodeGenerator* generator = static_cast<CodeGenerator*>( user_data );
    std::string tagName = std::string( name );
    std::map<std::string,std::string> tagAttrs;
    int i = 0;

    while( attrs[ i ] != NULL ){
        std::string attrKey( attrs[ i ] );
        std::string attrValue( attrs[ i + 1 ] );

        tagAttrs.insert( std::make_pair( attrKey, attrValue ) );

        i += 2;
    }

    generator->start_element( tagName, tagAttrs );
}

void CodeGenerator::end_element_handler( void* userData, const XML_Char* name ){
    CodeGenerator* generator = static_cast<CodeGenerator*>( userData );
    std::string tagName = std::string( name );

    generator->end_element( tagName );
}

void CodeGenerator::generateProxyClasses( bool outputToFile, const std::string& output_directory ){
    for( cppgenerate::Class c : m_proxyClasses ){
        if( outputToFile ){
            std::string headerFilename = output_directory + c.getName() + std::string( ".h" );
            std::string implFilename = output_directory + c.getName() + std::string( ".cpp" );
            std::ofstream header;
            std::ofstream impl;

            header.open( headerFilename );
            impl.open( implFilename );

            c.print( header, impl );
        }else{
            c.printAsHeaderOnly( std::cout );
        }
    }

}

void CodeGenerator::generateAdapterClasses( bool outputToFile, const std::string& output_directory ){
    for( cppgenerate::Class c : m_adapterClasses ){
        if( outputToFile ){
            std::string headerFilename = output_directory + c.getName() + std::string( ".h" );
            std::string implFilename = output_directory + c.getName() + std::string( ".cpp" );
            std::ofstream header;
            std::ofstream impl;

            header.open( headerFilename );
            impl.open( implFilename );

            c.print( header, impl );
        }else{
            c.printAsHeaderOnly( std::cout );
        }
    }

    for( cppgenerate::Class c : m_adapteeClasses ){
        if( outputToFile ){
            std::string headerFilename = output_directory + c.getName() + std::string( ".h" );
            std::ofstream header;

            header.open( headerFilename );

            c.printAsHeaderOnly( header );
        }else{
            c.printAsHeaderOnly( std::cout );
        }
    }
}

std::string CodeGenerator::getTemplateArgsFromSignature( SignatureIterator it ){
    std::string ret;

    do{
        TypeInfo info( it.type() );
        for( std::string include_file : info.includeFilesForType() ){
            m_adapteeClasses[ m_adapteeClasses.size() - 1 ].addSystemInclude( include_file );
            m_adapterClasses[ m_adapterClasses.size() - 1 ].addSystemInclude( include_file );
        }

        if( ret.length() > 0 ){
            ret += ",";
        }

        /* Special case: if we have a dictionary, we will recurse twice, so don't
         * fill in anything for the cpp type.  This is also because a dict
         * looks like a{ss}, which is at first glance indistinguishable from
         * an array(given the 'a' type)
         */
        if( !it.is_dict() ){
            ret += info.cppType();
        }

        if( info.isTemplated() ){
            if( !it.is_dict() ) ret += "<";
            ret += getTemplateArgsFromSignature( it.recurse() );
            if( !it.is_dict() ) ret += ">";
        }
    }while( it.next() );

    return ret;
}

void CodeGenerator::handle_node_tag( std::map<std::string,std::string>& tagAttrs ){
    cppgenerate::Class newclass;
    cppgenerate::Class newAdapterClass;
    cppgenerate::Class newAdapteeClass;
    std::string dest;
    std::string path;

    if( tagAttrs.find( "gen-namespace" ) != tagAttrs.end() ){
        newclass.setNamespace( tagAttrs.find( "gen-namespace" )->second );
    }

    if( tagAttrs.find( "dest" ) != tagAttrs.end() ){
        dest = "\"" + tagAttrs[ "dest" ] + "\"";
    }else{
        std::cerr << "WARNING: Did not find 'dest' in XML for node \'"
                  << "node" << "\'.  Line:"
                  << XML_GetCurrentLineNumber( m_parser )
                  << std::endl;
    }

    if( tagAttrs.find( "name" ) != tagAttrs.end() ){
        path = "\"" + tagAttrs[ "name" ] + "\"";
    }else if( tagAttrs.find( "path" ) != tagAttrs.end() ){
        path = "\"" + tagAttrs[ "path" ] + "\"";
    }else{
        std::cerr << "WARNING: Did not find 'path' or 'name' in xml for node \'"
                  << "node" << "\'.  Line:"
                  << XML_GetCurrentLineNumber( m_parser )
                  << std::endl;
    }

    if( tagAttrs.find( "cppname" ) != tagAttrs.end() ){
        std::string cppname = tagAttrs[ "cppname" ];
        std::replace( cppname.begin(), cppname.end(), '.', '_' );
        newclass.setName( cppname + "Proxy" );
        newAdapterClass.setName( cppname + "Adapter" );
        newAdapteeClass.setName( cppname + "Adaptee" );
    }else{
        newclass.setName( "NONAME_Proxy" );
        newAdapterClass.setName( "NONAME_Adapter" );
        newAdapteeClass.setName( "NONAME_Adaptee" );
    }


    /* Add new proxy class */
    newclass.addSystemInclude( "dbus-cxx.h" )
      .addSystemInclude( "stdint.h" )
      .addSystemInclude( "string" )
      .addSystemInclude( "memory" )
      .addParentClass( "DBus::ObjectProxy", cppgenerate::AccessModifier::PUBLIC, "conn, dest, path" );

    newclass.addMethod( cppgenerate::Method::create()
        .setAccessModifier( cppgenerate::AccessModifier::PUBLIC )
        .setName( "create" )
        .setStatic( true )
        .setReturnType( "std::shared_ptr<" + newclass.getName() + ">" )
        .addCode( cppgenerate::CodeBlock::create()
            .addLine( "return std::shared_ptr<" + newclass.getName() + ">( new " + newclass.getName() + "( conn, dest, path, signalCallingThread ) );" ) )
        .addArgument( cppgenerate::Argument::create()
          .setType( "std::shared_ptr<DBus::Connection>" )
          .setName( "conn" ) )
        .addArgument( cppgenerate::Argument::create()
          .setType( "std::string" )
          .setName( "dest" )
          .setDefaultValue( dest ) )
        .addArgument( cppgenerate::Argument::create()
          .setType( "std::string" )
          .setName( "path" )
          .setDefaultValue( path ) )
        .addArgument( cppgenerate::Argument::create()
          .setType( "DBus::ThreadForCalling" )
          .setName( "signalCallingThread" )
          .setDefaultValue( "DBus::ThreadForCalling::DispatcherThread" ) ) );

    m_currentProxyConstructor = cppgenerate::Constructor::create()
        .setAccessModifier( cppgenerate::AccessModifier::PROTECTED )
        .addArgument( cppgenerate::Argument::create()
          .setType( "std::shared_ptr<DBus::Connection>" )
          .setName( "conn" ) )
        .addArgument( cppgenerate::Argument::create()
          .setType( "std::string" )
          .setName( "dest" )
          .setDefaultValue( dest ) )
        .addArgument( cppgenerate::Argument::create()
          .setType( "std::string" )
          .setName( "path" )
          .setDefaultValue( path ) )
        .addArgument( cppgenerate::Argument::create()
          .setType( "DBus::ThreadForCalling" )
          .setName( "signalCallingThread" )
          .setDefaultValue( "DBus::ThreadForCalling::DispatcherThread" ) );

    m_proxyClasses.push_back( newclass );

    /* Add new adapter class */
    newAdapterClass.addSystemInclude( "dbus-cxx.h" )
      .addSystemInclude( "stdint.h" )
      .addSystemInclude( "string" )
      .addSystemInclude( "memory" )
      .addLocalInclude( newAdapteeClass.getName() + ".h" )
      .addParentClass( "DBus::Object", cppgenerate::AccessModifier::PUBLIC, "path" );

    newAdapterClass.addMethod( cppgenerate::Method::create()
        .setAccessModifier( cppgenerate::AccessModifier::PUBLIC )
        .setName( "create" )
        .setStatic( true )
        .setDocumentation( "Create an adapter class that you can register with a connection later" )
        .setReturnType( "std::shared_ptr<" + newAdapterClass.getName() + ">" )
        .addCode( cppgenerate::CodeBlock::create()
            .addLine( "return std::shared_ptr<" + newAdapterClass.getName() + ">( new " + newAdapterClass.getName() + "( adaptee, path ) );" ) )
        .addArgument( cppgenerate::Argument::create()
          .setType( newAdapteeClass.getName() + "*" )
          .setName( "adaptee" ) )
        .addArgument( cppgenerate::Argument::create()
          .setType( "std::string" )
          .setName( "path" )
          .setDefaultValue( path ) ) );

    newAdapterClass.addMethod( cppgenerate::Method::create()
        .setAccessModifier( cppgenerate::AccessModifier::PUBLIC )
        .setName( "create" )
        .setStatic( true )
        .setDocumentation( "Create an adapter class that is registered with the given connection" )
        .setReturnType( "std::shared_ptr<" + newAdapterClass.getName() + ">" )
        .addCode( cppgenerate::CodeBlock::create()
            .addLine( "std::shared_ptr<" + newAdapterClass.getName() + "> " +
                   "new_adaptee = std::shared_ptr<" +
                      newAdapterClass.getName() +
                   ">( new " + newAdapterClass.getName() + "( adaptee, path ) );" )
            .addLine( "if( connection ){ " )
            .addLine( "  if( connection->register_object( new_adaptee, calling_thread ) != DBus::RegistrationStatus::Success ){" )
            .addLine( "    return std::shared_ptr<" + newAdapterClass.getName() + ">();" )
            .addLine( "  }" )
            .addLine( "}" )
            .addLine( "return new_adaptee;" ) )
        .addArgument( cppgenerate::Argument::create()
          .setType( "std::shared_ptr<DBus::Connection>" )
          .setName( "connection" ) )
        .addArgument( cppgenerate::Argument::create()
          .setType( "DBus::ThreadForCalling" )
          .setName( "calling_thread" ) )
        .addArgument( cppgenerate::Argument::create()
          .setType( newAdapteeClass.getName() + "*" )
          .setName( "adaptee" ) )
        .addArgument( cppgenerate::Argument::create()
          .setType( "std::string" )
          .setName( "path" )
          .setDefaultValue( path ) ) );

    m_currentAdapterConstructor.addCode( cppgenerate::CodeBlock::create()
        .addLine( "std::shared_ptr<DBus::MethodBase> temp_method;" ) )
        .setAccessModifier( cppgenerate::AccessModifier::PROTECTED );

    m_currentAdapterConstructor.addArgument( cppgenerate::Argument::create()
        .setType( newAdapteeClass.getName() + "*" )
        .setName( "adaptee" ) )
        .addArgument( cppgenerate::Argument::create()
          .setType( "std::string" )
          .setName( "path" )
          .setDefaultValue( path )  );

    m_adapterClasses.push_back( newAdapterClass );
    m_adapteeClasses.push_back( newAdapteeClass );
}

void CodeGenerator::handle_method_tag( std::map<std::string,std::string>& tagAttrs ){
    m_argNum = 0;
    m_returnName = "";
    if( tagAttrs.find( "name" ) == tagAttrs.end() ){
        std::cerr << "ERROR: No name for method found; ignoring.  Line:"
                  << XML_GetCurrentLineNumber( m_parser ) << "\n";
        return;
    }

    std::string methodName = tagAttrs[ "name" ];
    std::replace( methodName.begin(), methodName.end(), '.', '_' );

    m_currentProxyMethod = cppgenerate::Method()
        .setName( methodName )
        .setAccessModifier( cppgenerate::AccessModifier::PUBLIC );

    m_currentAdapteeMethod = cppgenerate::Method()
        .setName( methodName )
        .setAccessModifier( cppgenerate::AccessModifier::PUBLIC )
        .setPureVirtual( true );
}

void CodeGenerator::handle_signal_tag( std::map<std::string,std::string>& tagAttrs ){
    m_argNum = 0;
    m_returnName = "";
    if( tagAttrs.find( "name" ) == tagAttrs.end() ){
        std::cerr << "ERROR: No name for signal found, ignoring.  Line:"
                  << XML_GetCurrentLineNumber( m_parser ) << "\n";
        return;
    }

    std::string signalName = tagAttrs[ "name" ];
    std::replace( signalName.begin(), signalName.end(), '.', '_' );

    m_currentSignal = SignalInfo( signalName );
}

void CodeGenerator::handle_arg_tag( std::string parentElement, std::map<std::string,std::string>& tagAttrs ){
    cppgenerate::Argument arg;

    if( tagAttrs.find( "direction" ) == tagAttrs.end() &&
        parentElement == "method" ){
        std::cerr << "WARNING: No direction for arg found(assuming in).  Line:"
                  << XML_GetCurrentLineNumber( m_parser ) << "\n";
    }

    DBus::Signature signature( tagAttrs[ "type" ] );

    DBus::Signature::iterator it = signature.begin();
    std::string typestr = getTemplateArgsFromSignature( it );

    arg.setType( typestr );
    if( tagAttrs[ "name" ].length() == 0 ){
        char buffer[10];
        snprintf( buffer, 10, "arg%d", m_argNum );
        arg.setName( std::string( buffer ) );
    }else{
        arg.setName( tagAttrs[ "name" ] );
    }

    if( tagAttrs[ "direction" ] == "out" && parentElement == "method" ){
        m_currentProxyMethod.setReturnType( typestr );
        m_currentAdapteeMethod.setReturnType( typestr );
        if( tagAttrs[ "name" ].length() ){
            m_returnName = tagAttrs[ "name" ];
        }
    }else if( parentElement == "method" ){
        m_currentProxyMethod.addArgument( arg );
        m_currentAdapteeMethod.addArgument( arg );
    }else if( parentElement == "signal" ){
        m_currentSignal.addArgument( arg );
    }

    m_argNum++;
}

void CodeGenerator::handle_property_tag( std::map<std::string, std::string> &tagAttrs ){
    std::string propertyName;
    std::string propertyType;
    std::string propertyAccess;
    bool hasError = false;

    if( tagAttrs[ "name" ].length() == 0 ){
        std::cerr << "ERROR: No name found for property; ignoring!  Line:"
                  << XML_GetCurrentLineNumber( m_parser ) << "\n";
        hasError = true;
    }

    if( tagAttrs[ "type" ].length() == 0 ){
        std::cerr << "ERROR: No type found for property; ignoring!  Line:"
                  << XML_GetCurrentLineNumber( m_parser ) << "\n";
        hasError = true;
    }

    if( tagAttrs[ "access" ].length() == 0 ){
        std::cerr << "ERROR: No access found for property; ignoring!  Line:"
                  << XML_GetCurrentLineNumber( m_parser ) << "\n";
        hasError = true;
    }

    if( hasError ){
        return;
    }

    propertyName = tagAttrs[ "name" ];
    propertyType = tagAttrs[ "type" ];
    propertyAccess = tagAttrs[ "access" ];

    // We treat properties like normal methods under the assumption that a property
    // will not be named the same as a method.
    DBus::Signature signature( propertyType );

    DBus::Signature::iterator it = signature.begin();
    std::string typestr = getTemplateArgsFromSignature( it );
    cppgenerate::Method proxyGetMethod = cppgenerate::Method::create()
            .setName( propertyName )
            .setReturnType( typestr )
            .setAccessModifier( cppgenerate::AccessModifier::PUBLIC );
    cppgenerate::Method proxySetMethod = cppgenerate::Method::create()
            .setName( "set_" + propertyName )
            .addArgument( cppgenerate::Argument::create()
                          .setName( propertyName )
                          .setType( typestr )
                          )
            .setAccessModifier( cppgenerate::AccessModifier::PUBLIC );

    if( propertyAccess.compare( "read" ) == 0 ){
        m_proxyClasses.data()[ m_proxyClasses.size() - 1 ]
            .addMethod( proxyGetMethod );
    }else if( propertyAccess.compare( "write" ) == 0 ){
        m_proxyClasses.data()[ m_proxyClasses.size() - 1 ]
            .addMethod( proxySetMethod );
    }else if( propertyAccess.compare( "readwrite" ) == 0 ){
        m_proxyClasses.data()[ m_proxyClasses.size() - 1 ]
            .addMethod( proxyGetMethod );
        m_proxyClasses.data()[ m_proxyClasses.size() - 1 ]
            .addMethod( proxySetMethod );
    }
}
