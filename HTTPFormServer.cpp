//
// HTTPFormServer.cpp
//
// This sample demonstrates the HTTPServer and HTMLForm classes.
//
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTMLForm.h"
#include "Poco/Net/PartHandler.h"
#include "Poco/Net/MessageHeader.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/CountingStream.h"
#include "Poco/NullStream.h"
#include "Poco/StreamCopier.h"
#include "Poco/Exception.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include <iostream>


using Poco::Net::ServerSocket;    		//This class implements a TCP server socket

using Poco::Net::HTTPRequestHandler;  	//The abstract base class for HTTPRequestHandlers created by HTTPServer.

using Poco::Net::HTTPRequestHandlerFactory; //A factory for HTTPRequestHandler objects.

using Poco::Net::HTTPServer; 			//A subclass of TCPServer that implements a full-featured multithreaded HTTP server.

using Poco::Net::HTTPServerRequest; 	//This abstract subclass of HTTPRequest is used for representing server-side HTTP requests

using Poco::Net::HTTPServerResponse; 	//This subclass of HTTPResponse is used for representing server-side HTTP responses

using Poco::Net::HTTPServerParams;    //This class is used to specify parameters to both the HTTPServer, as well as to HTTPRequestHandler objects

using Poco::Net::MessageHeader;			//A collection of name-value pairs that are used in various internet protocols like HTTP and SMTP.

using Poco::Net::HTMLForm;				//HTMLForm is a helper class for working with HTML forms, both on the client and on the server side

using Poco::Net::NameValueCollection;   //A collection of name-value pairs that are used in various internet protocols like HTTP and SMTP.

using Poco::Util::ServerApplication;  //A subclass of the Application class that is used for implementing server applications

using Poco::Util::Application; 		 //The Application class implements the main subsystem in a process

using Poco::Util::Option;    //This class represents and stores the properties of a command line option

using Poco::Util::OptionSet; //A collection of Option objects

using Poco::Util::HelpFormatter; //This class formats a help message from an OptionSet.

using Poco::CountingInputStream; //This stream counts all characters and lines going through it. This is useful for lexers and parsers that need to determine the current position in the stream.

using Poco::NullOutputStream;  //This stream discards all characters written to it.

using Poco::StreamCopier;     //This class provides static methods to copy the contents from one stream into another.


class MyPartHandler: public Poco::Net::PartHandler  //creating class and inherting Poco::Net::PartHandler
{
public:
	MyPartHandler():  //constructor is automatically called when an object is created.
		_length(0)
	{
	}
	
	void handlePart(const MessageHeader& header, std::istream& stream)  //handlepart:- Information about the part can be extracted from the given message header
	{															//messageheader:- A collection of name-value pairs that are used in various internet protocols like HTTP and SMTP
		                                                        //stream:- The content of the part can be read from stream.
		
		_type = header.get("Content-Type", "(unspecified)"); //type storing what type of data is recived 
		
		if (header.has("Content-Disposition"))  //checking if the content is expected to be displayed inline in the browser
		{
			std::string disp;
			NameValueCollection params;  //A collection of name-value pairs that are used in various internet protocols like HTTP and SMTP.
			MessageHeader::splitParameters(header["Content-Disposition"], disp, params);  //Splits the given string into a value and a collection of parameters
			_name = params.get("name", "(unnamed)"); //get takes optonal header and body
			_fileName = params.get("filename", "(unnamed)"); 
		}
		
		CountingInputStream istr(stream);
		NullOutputStream ostr;
		StreamCopier::copyStream(istr, ostr);
		_length = istr.chars();
	}
	
	int length() const
	{
		return _length;
	}
	
	const std::string& name() const
	{
		return _name;
	}

	const std::string& fileName() const
	{
		return _fileName;
	}
	
	const std::string& contentType() const
	{
		return _type;
	}
	
private:
	int _length;
	std::string _type;
	std::string _name;
	std::string _fileName;
};


class FormRequestHandler: public HTTPRequestHandler
	/// Return a HTML document 
{
public:
	FormRequestHandler() 
	{
	}
	
	void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)  //handeling request and response
	{
		Application& app = Application::instance();
		app.logger().information("Request from " + request.clientAddress().toString());

		MyPartHandler partHandler;
		HTMLForm form(request, request.stream(), partHandler);

		response.setChunkedTransferEncoding(true);
		response.setContentType("text/html");

		std::ostream& ostr = response.send();
		
		ostr <<
			"<html>\n"
			"<head>\n"
			"<title>POCO Form Server Sample</title>\n"
			"</head>\n"
			"<body>\n"
			"<h1>POCO Form Server Sample</h1>\n"
			"<h2>GET Form</h2>\n"
			"<form method=\"GET\" action=\"/form\">\n"
			"<input type=\"text\" name=\"text\" size=\"31\">\n"
			"<input type=\"submit\" value=\"GET\">\n"
			"</form>\n"
			"<h2>POST Form</h2>\n"
			"<form method=\"POST\" action=\"/form\">\n"
			"<input type=\"text\" name=\"text\" size=\"31\">\n"
			"<input type=\"submit\" value=\"POST\">\n"
			"</form>\n"
			"<h2>File Upload</h2>\n"
			"<form method=\"POST\" action=\"/form\" enctype=\"multipart/form-data\">\n"
			"<input type=\"file\" name=\"file\" size=\"31\"> \n"
			"<input type=\"submit\" value=\"Upload\">\n"
			"</form>\n"
			;
			
		ostr << "<h2>Request</h2><p>\n";
		ostr << "Get or Post: " << request.getMethod() << "<br>\n";
		ostr << "URI: " << request.getURI() << "<br>\n";
		NameValueCollection::ConstIterator it = request.begin(); //Returns an iterator pointing to the begin of the name-value pair collection.
		NameValueCollection::ConstIterator end = request.end();  //Returns an iterator pointing to the end of the name-value pair collection.
		for (; it != end; ++it)
		{
			ostr << it->first << ": " << it->second << "<br>\n";
		}
		ostr << "</p>";

		if (!form.empty())
		{
			ostr << "<h2>Form</h2><p>\n";
			it = form.begin();
			end = form.end();
			for (; it != end; ++it)
			{
				ostr << it->first << ": " << it->second << "<br>\n";
			}
			ostr << "</p>";
		}
		
		if (!partHandler.name().empty())
		{
			ostr << "<h2>Upload</h2><p>\n";
			ostr << "Name: " << partHandler.name() << "<br>\n";
			ostr << "File Name: " << partHandler.fileName() << "<br>\n";
			ostr << "Type: " << partHandler.contentType() << "<br>\n";
			ostr << "Size: " << partHandler.length() << "<br>\n";
			ostr << "</p>";
		}
		ostr << "</body>\n";
	}
};


class FormRequestHandlerFactory: public HTTPRequestHandlerFactory
{
public:
	FormRequestHandlerFactory()
	{
	}

	HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request)  //Creates a new request handler for the given HTTP request.
	{
		return new FormRequestHandler;
	}
};


class HTTPFormServer: public Poco::Util::ServerApplication
	/// The main application class.
	///
	/// This class handles command-line arguments and
	/// configuration files.
	/// Start the HTTPFormServer executable with the help
	/// option (/help on Windows, --help on Unix) for
	/// the available command line options.
	///
	/// To use the sample configuration file (HTTPFormServer.properties),
	/// copy the file to the directory where the HTTPFormServer executable
	/// resides. If you start the debug version of the HTTPFormServer
	/// (HTTPFormServerd[.exe]), you must also create a copy of the configuration
	/// file named HTTPFormServerd.properties. In the configuration file, you
	/// can specify the port on which the server is listening (default
	/// 9980) and the format of the date/Form string sent back to the client.
	///
	/// To test the FormServer you can use any web browser (http://localhost:9980/).
{
public:
	HTTPFormServer(): _helpRequested(false)   //Creates the HTTPFormServer() This class handles command-line arguments
	{
	}
	
	~HTTPFormServer()  //distroyes the HTTPFormserver()
	{
	}

protected:
	void initialize(Application& self)
	{
		loadConfiguration(); 	// load default configuration files, if present
		ServerApplication::initialize(self);
	}
		
	void uninitialize()
	{
		ServerApplication::uninitialize();
	}

	void defineOptions(OptionSet& options)   //defining the options for command-line
	{
		ServerApplication::defineOptions(options);
		
		options.addOption(
			Option("help", "h", "display help information on command line arguments")
				.required(false)
				.repeatable(false));
	}

	void handleOption(const std::string& name, const std::string& value) //accepting options
	{
		ServerApplication::handleOption(name, value);

		if (name == "help")
			_helpRequested = true;
	}

	void displayHelp()  
	{
		HelpFormatter helpFormatter(options());
		helpFormatter.setCommand(commandName());
		helpFormatter.setUsage("OPTIONS");
		helpFormatter.setHeader("A web server that shows how to work with HTML forms.");
		helpFormatter.format(std::cout);
	}

	int main(const std::vector<std::string>& args)
	{
		if (_helpRequested)
		{
			displayHelp();
		}
		else
		{
			unsigned short port = (unsigned short) config().getInt("HTTPFormServer.port", 9980);
			
			// set-up a server socket
			ServerSocket svs(port);
			// set-up a HTTPServer instance
			HTTPServer srv(new FormRequestHandlerFactory, svs, new HTTPServerParams); //used to specify parameters to both the HTTPServer, as well as to HTTPRequestHandler objects
			// start the HTTPServer
			srv.start();
			// wait for CTRL-C or kill
			waitForTerminationRequest();
			// Stop the HTTPServer
			srv.stop();
		}
		return Application::EXIT_OK;
	}
	
private:
	bool _helpRequested;
};


int main(int argc, char** argv)
{
	HTTPFormServer app;
	return app.run(argc, argv);
}
