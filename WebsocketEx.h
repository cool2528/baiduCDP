#ifndef __CWEBSOCKETEX__
#define  __CWEBSOCKETEX__
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
typedef websocketpp::client<websocketpp::config::asio_client> client;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
//退出发送的消息的类型,通过我们的配置
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
class CWebsocketEx
{
public:
	CWebsocketEx();
	~CWebsocketEx();
public:
	static void on_MessageCall_Back(client* c, websocketpp::connection_hdl hdl, message_ptr msg);
};
#endif //__CWEBSOCKETEX__
