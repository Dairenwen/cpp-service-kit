#include "BreezeRPC.hpp"

Channels::Channels() : _idx(0) {}

void Channels::insert(const std::string &addr)
{
    auto channel = std::make_shared<brpc::Channel>();
    brpc::ChannelOptions options;
    options.protocol = "baidu_std";
    options.timeout_ms = 30000;
    if (channel->Init(addr.c_str(), &options) != 0)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(_mtx);
    if (_channels.find(addr) == _channels.end())
    {
        _channel_list.push_back(channel);
        _channels[addr] = channel;
    }
}

void Channels::remove(const std::string &addr)
{
    std::lock_guard<std::mutex> lock(_mtx);
    if (_channels.find(addr) != _channels.end())
    {
        for (auto it = _channel_list.begin(); it != _channel_list.end(); ++it)
        {
            if ((*it) == _channels[addr])
            {
                _channel_list.erase(it);
                break;
            }
        }
        _channels.erase(addr);
    }
}

Channels::ChannelPtr Channels::select()
{
    std::lock_guard<std::mutex> lock(_mtx);
    if (_channel_list.empty())
    {
        return nullptr;
    }

    return _channel_list[_idx++ % _channel_list.size()];
}

void SvcChannels::setWatch(const std::string &svc_name)
{
    std::lock_guard<std::mutex> lock(_mtx);
    if (_maps.find(svc_name) == _maps.end())
    {
        _maps[svc_name] = std::make_shared<Channels>();
    }
}

void SvcChannels::addNode(const std::string &svc_name, const std::string &addr)
{
    std::lock_guard<std::mutex> lock(_mtx);
    if (_maps.find(svc_name) != _maps.end())
    {
        _maps[svc_name]->insert(addr);
    }
    else
    {
        _maps[svc_name] = std::make_shared<Channels>();
        _maps[svc_name]->insert(addr);
    }
}

void SvcChannels::delNode(const std::string &svc_name, const std::string &addr)
{
    std::lock_guard<std::mutex> lock(_mtx);
    if (_maps.find(svc_name) != _maps.end())
    {
        _maps[svc_name]->remove(addr);
    }
}

Channels::ChannelPtr SvcChannels::getNode(const std::string &svc_name)
{
    std::lock_guard<std::mutex> lock(_mtx);
    if (_maps.find(svc_name) != _maps.end())
    {
        return _maps[svc_name]->select();
    }
    return nullptr;
}

google::protobuf::Closure *ClosureFactor::create(callback_t cb)
{
    auto obj = std::make_shared<Object>();
    obj->callback = cb;
    return google::protobuf::NewCallback(&ClosureFactor::asyncCallback, obj);
}

void ClosureFactor::asyncCallback(const Object::ptr obj)
{
    if (obj->callback)
    {
        obj->callback();
    }
}

std::shared_ptr<brpc::Server> RpcServerFactory::create(int port, google::protobuf::Service *svc)
{
    if (svc == nullptr)
    {
        return nullptr;
    }

    auto server = std::make_shared<brpc::Server>();

    // 服务对象由于 brpc::Server 接管，所以必须在堆上创建，不能在栈上创建
    if (server->AddService(svc, brpc::SERVER_OWNS_SERVICE) != 0)
    {
        LOG(ERROR) << "Fail to add service";
        return nullptr;
    }

    brpc::ServerOptions options;
    options.idle_timeout_sec = -1;

    if (server->Start(port, &options) != 0)
    {
        LOG(ERROR) << "Fail to start server";
        return nullptr;
    }
    return server;
}
