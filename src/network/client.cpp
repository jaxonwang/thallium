#include "client.hpp"

#include <boost/asio.hpp>
#include <memory>

#include "heartbeat.hpp"
#include "logging.hpp"

_THALLIUM_BEGIN_NAMESPACE

using namespace std;
using namespace std::placeholders;
using asio_tcp = boost::asio::ip::tcp;
namespace asio = boost::asio;

typedef typename asio_tcp::resolver::results_type endpoints_type ;
typedef endpoints_type::endpoint_type endpoint_type;

endpoints_type try_resovle(execution_context & ctx, const string &hostname, const int port){
    asio_tcp::resolver rslver{ctx};

    endpoints_type eps;

    try {
        eps = rslver.resolve(hostname, to_string(port));
    } catch (const std::system_error &e) {
        TI_FATAL(format("Unable to resolve {}, error: {}", hostname, e.what()));
        throw e;
    }
    return eps;
}

template<class EndPoints>
void try_connect(asio_tcp::socket & asio_s, const EndPoints &eps){

    endpoint_type ep;
    try {
        ep = boost::asio::connect(asio_s, eps);
        // copy the connected
    } catch (const std::system_error &e) {
        TI_FATAL(format("Connect failed: {}", e.what()));
        throw e;
    }
    TI_DEBUG(format("Successfully connect to {}", ep.address().to_string()));
}

void AsyncClient::init_connection(asio_tcp::socket && asio_s){
    // just return 0 as conn id
    typedef Connection::receive_callback receive_callback;
    typedef Connection::event_callback event_callback;
    receive_callback f = std::bind(&AsyncClient::receive, this, 0, _1, _2);
    event_callback e_f = bind(&AsyncClient::event, this, 0, _1);
    ClientConnection *new_con_ptr;
    if (withheartbeat)
        new_con_ptr =
            new ClientConnectionWithHeartbeat(_context, move(asio_s), f, e_f);
    else
        new_con_ptr = new ClientConnection(_context, move(asio_s), f, e_f);
    holding.reset(new_con_ptr);
}

// will throw
AsyncClient::AsyncClient(execution_context &ctx, const ti_socket_t &s,
                         const bool heartbeat)
    : _socket(s), _context(ctx), withheartbeat(heartbeat) {
    asio_tcp::endpoint ep{_socket.addr, _socket.port};
    asio_tcp::socket asio_s{_context};
    // why asio must accept endpoint list
    vector<asio_tcp::endpoint> eps;
    eps.push_back(ep);

    try_connect(asio_s, eps);
    init_connection(move(asio_s));
}

AsyncClient::AsyncClient(execution_context &ctx, const string &hostname,
                         const unsigned short port, const bool heartbeat)
    : _context(ctx), withheartbeat(heartbeat) {
    asio_tcp::resolver rslver{_context};

    auto eps = try_resovle(ctx, hostname, port);

    asio_tcp::socket asio_s{_context};
    try_connect(asio_s, eps);
    auto ep = asio_s.remote_endpoint();
    _socket.addr = ep.address().to_v4();
    _socket.port = port;

    init_connection(move(asio_s));
}

void AsyncClient::start() { holding->do_receive_message(); }

ti_socket_t AsyncClient::client_socket() { return _socket; }

void AsyncClient::send(const int, message::ZeroCopyBuffer &&message) {
    holding->do_send_message(move(message));
}

void AsyncClient::receive(const int conn_id, const char *buf,
                          const size_t length) {
    upper->receive(conn_id, buf, length);
}

void AsyncClient::event(const int conn_id, const message::ConnectionEvent &e) {
    upper->event(conn_id, e);
}

void AsyncClient::disconnect(const int) { holding->connection_close(); }

void AsyncClient::stop() {}

MultiClient::MultiClient(execution_context &ctx, const address_book_t &book)
    : _context(ctx), address_book(book), conmanager(ctx), established() {
        // in principle expose "this" in consturctor is dangerous,
        // but now it works well....
        // may need future refactoring
        conmanager.set_upper(this); 
    }

void MultiClient::do_connect(const int conn_id){
    const auto &host_and_port = address_book[conn_id];
    const string & hostname = host_and_port.first;
    const int & port = host_and_port.second;

    auto eps = try_resovle(_context, hostname, port);
    asio_tcp::socket asio_s{_context};
    try_connect(asio_s, eps);

    // insert into connection manager
    conmanager.new_connection(move(asio_s), conn_id);
    // set established
    established.insert(conn_id);
}

void MultiClient::start(){
    _context.post(bind(&MultiClient::event, this, -1, message::ConnectionEvent::start));
}

void MultiClient::stop(){ }

void MultiClient::send(const int conn_id, message::ZeroCopyBuffer &&msg){
    if(established.count(conn_id) == 0)
        do_connect(conn_id);
    conmanager.send(conn_id, move(msg));
}

void MultiClient::disconnect(const int conn_id){
    conmanager.disconnect(conn_id);
    established.erase(conn_id);
}

void MultiClient::receive(const int conn_id, const char * buf, const size_t length){
    upper->receive(conn_id, buf, length);
}

void MultiClient::event(const int conn_id, const message::ConnectionEvent &e) {
    upper->event(conn_id, e);
}

_THALLIUM_END_NAMESPACE
