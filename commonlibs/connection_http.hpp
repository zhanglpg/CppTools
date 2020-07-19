#ifndef COMMONLIBS_HTTP_CONNECTION_HPP
#define COMMONLIBS_HTTP_CONNECTION_HPP

#include "commonlibs/probemessage.hpp"
#include "commonlibs/connection.hpp"
#include <sstream>

namespace commonlibs {

class synchronized_http_client : public commonlibs::connection_tl 
{
public: 
	synchronized_http_client(boost::asio::io_service &ios_  ,const std::string &host_ , 
		const std::string&  service_,  const std::string &filename_) : 
		commonlibs::connection_tl(ios_),
		filename(filename_) , hostname(host_) , 
		service(service_) 

	{
		// start the session

	}

	int send()
	{
		int ret = 0 ;
		try {
			boost::system::error_code ec_ ;
			sync_connect_host(hostname ,
				service,
				ec_) ;
			handle_connect(ec_) ;
		}
		catch(const boost::system::system_error &be_) 
		{
			//be_.code() == 
			ret = be_.code().value() ;
			std::cerr << be_.what() << std::string("--synchronized_connector::start()") << std::endl ;
		}
		catch(const  std::exception & e_)
		{
			ret = -1 ;
			std::cerr << e_.what() << std::string("--synchronized_connector::start()") << std::endl ;
		}
		close() ;
		return ret ;
	}

	const std::string & get_header() const 
	{
		return s_header;
	}
	const std::string &get_body() const 
	{
		
		return s_body ;
	}
	void handle_connect(boost::system::error_code& e)
	{

		stringstream ssheader ;
		stringstream ssbody ;
	// Form the request. We specify the "Connection: close" header so that the
		// server will close the socket after transmitting the response. This will
		// allow us to treat all data up until the EOF as the content.
		boost::asio::streambuf request;
		std::ostream request_stream(&request);
		request_stream << "GET " << filename << " HTTP/1.0\r\n";
		request_stream << "Host: " << hostname << "\r\n";
		request_stream << "Accept: */*\r\n";
		request_stream << "Connection: close\r\n\r\n";

		// Send the request.
		boost::asio::write(socket(), request);

		// Read the response status line.
		
		boost::asio::streambuf response;
		boost::asio::read_until(socket(), response, "\r\n");

		// Check that response is OK.
		std::istream response_stream(&response);
		std::string http_version;
		response_stream >> http_version;
		unsigned int status_code;
		response_stream >> status_code;
		std::string status_message;
		std::getline(response_stream, status_message);
		if (!response_stream || http_version.substr(0, 5) != "HTTP/")
		{
			e = boost::system::error_code(boost::asio::error::bad_descriptor) ;
			throw boost::system::system_error(e) ;
		}
		if (status_code != 200)
		{
			e = boost::system::error_code(boost::asio::error::bad_descriptor);
			std::cout << "Response returned with status code " << status_code << "\n";
			throw boost::system::system_error(e) ;
		}

		// Read the response headers, which are terminated by a blank line.
		boost::asio::read_until(socket(), response, "\r\n\r\n");

		// Process the response headers.
		std::string header;

		while (std::getline(response_stream, header) && header != "\r") {
		  //std::cout << header << "\n";
			ssheader << header << "\n" ;
		}

		// Write whatever content we already have to output.
		if (response.size() > 0) {
		  //std::cout << &response;
		  ssbody << &response ;
		}

		
		// Read until EOF, writing data to output as we go.
		while (boost::asio::read(socket(), response,
			boost::asio::transfer_at_least(1), e)) {
		 // std::cout << &response;
				ssbody << &response ;
		}
		s_header = ssheader.str() ;
		s_body  = ssbody.str() ;
		if (e != boost::asio::error::eof)
		  throw boost::system::system_error(e);
	}

private:
	std::string filename , hostname;
	std::string service ;
	std::string s_header ;
	std::string s_body ;

} ; 
}
#endif
