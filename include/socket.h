//
// C++ Interface: socket
//
// Description: Socket class, Source: http://tldp.org/LDP/LG/issue74/tougher.html#3.1
//
//
// Author: Jens Prager, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
// Usage:
//Simple Server Main /////////////////////////////////////////////
/*#include "Socket.h"
#include <string>

int main ( int argc, int argv[] )
{
  std::cout << "running....\n";
  try
  {
      // Create the socket
    ServerSocket server ( 30000 );
    while ( true )
    {
      ServerSocket new_sock;
      server.accept ( new_sock );
      try
      {
        while ( true )
        {
          std::string data;
          new_sock >> data;
          new_sock << data;
        }
      }
      catch ( SocketException& ) {}
    }
  }
  catch ( SocketException& e )
  {
    std::cout << "Exception was caught:" << e.description() << "\nExiting.\n";
  }
  return 0;
}*/
//Simple Client Main /////////////////////////////////////////////
/*#include "Socket.h"
#include <iostream>
#include <string>

int main ( int argc, int argv[] )
{
  try
  {
    ClientSocket client_socket ( "localhost", 30000 );
    std::string reply;
    try
    {
      client_socket << "Test message.";
      client_socket >> reply;
    }
    catch ( SocketException& ) {}
    std::cout << "We received this response from the server:\n\"" << reply << "\"\n";;
  }
  catch ( SocketException& e )
  {
    std::cout << "Exception was caught:" << e.description() << "\n";
  }
  return 0;
}*/
///////////////////////////////////////////////////////////////////

#ifndef Socket_class
#define Socket_class

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>


const int MAXHOSTNAME = 200;
const int MAXCONNECTIONS = 5;
const int MAXRECV = 500;

// Definition of the Socket class
class Socket
{
  public:
    Socket();
    virtual ~Socket();

  // Server initialization
    bool create();
    bool bind ( const int port );
    bool listen() const;
    bool accept ( Socket& ) const;

  // Client initialization
    bool connect ( const std::string host, const int port );

  // Data Transimission
    bool send ( const std::string ) const;
    int recv ( std::string& ) const;


    void set_non_blocking ( const bool );

    bool is_valid() const { return m_sock != -1; }

  private:

    int m_sock;
    sockaddr_in m_addr;


};

// SocketException class
class SocketException
{
  public:
    SocketException ( std::string s ) : m_s ( s ) {};
    ~SocketException (){};

    std::string description() { return m_s; }

  private:

    std::string m_s;

};

// Definition of the ServerSocket class
class ServerSocket : private Socket
{
  public:

    ServerSocket ( int port );
    ServerSocket (){};
    virtual ~ServerSocket();

    const ServerSocket& operator << ( const std::string& ) const;
    const ServerSocket& operator >> ( std::string& ) const;

    void accept ( ServerSocket& );

};

// Definition of the ClientSocket class
class ClientSocket : private Socket
{
  public:

    ClientSocket ( std::string host, int port );
    virtual ~ClientSocket(){};

    const ClientSocket& operator << ( const std::string& ) const;
    const ClientSocket& operator >> ( std::string& ) const;

};


#endif
