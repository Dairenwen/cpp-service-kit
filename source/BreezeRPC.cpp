#include "BreezeRPC.hpp"

class ClosureFactor::CallbackClosure final : public google::protobuf::Closure
{
public:
    explicit CallbackClosure(callback_t callback)
        : _object(std::make_shared<Object>())
    {
        _object->callback = std::move(callback);
    }

    void Run() override
    {
        auto object = std::move(_object);
        ClosureFactor::asyncCallback(object);
        delete this;
    }

private:
    Object::ptr _object;
};

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
        _channel_list.push_back(addr);
    }
    _channels[addr] = std::move(channel);
}

void Channels::remove(const std::string &addr)
{
    std::lock_guard<std::mutex> lock(_mtx);
    _channels.erase(addr);
    _channel_list.erase(
        std::remove(_channel_list.begin(), _channel_list.end(), addr),
        _channel_list.end());
    if (_channel_list.empty())
    {
        _idx = 0;
    }
    else
    {
        _idx %= _channel_list.size();
    }
}

Channels::ChannelPtr Channels::select()
{
    std::lock_guard<std::mutex> lock(_mtx);
    if (_channel_list.empty())
    {
        return nullptr;
    }

    const std::string &addr = _channel_list[_idx++ % _channel_list.size()];
    return _channels.at(addr);
}

std::optional<std::string> Channels::selectAddr()
{
    std::lock_guard<std::mutex> lock(_mtx);
    if (_channel_list.empty())
    {
        return std::nullopt;
    }

    return _channel_list[_idx++ % _channel_list.size()];
}

Channels::ptr SvcChannels::_channels(const std::string &svc_name)
{
    std::lock_guard<std::mutex> lock(_mtx);
    const auto it = _maps.find(svc_name);
    return it == _maps.end() ? nullptr : it->second;
}

void SvcChannels::setWatch(const std::string &svc_name)
{
    std::lock_guard<std::mutex> lock(_mtx);
    _maps.try_emplace(svc_name, std::make_shared<Channels>());
}

void SvcChannels::addNode(const std::string &svc_name,
                          const std::string &addr)
{
    const auto channels = _channels(svc_name);
    if (channels)
    {
        channels->insert(addr);
    }
}

void SvcChannels::delNode(const std::string &svc_name,
                          const std::string &addr)
{
    const auto channels = _channels(svc_name);
    if (channels)
    {
        channels->remove(addr);
    }
}

Channels::ChannelPtr SvcChannels::getNode(const std::string &svc_name)
{
    const auto channels = _channels(svc_name);
    return channels ? channels->select() : nullptr;
}

std::optional<std::string> SvcChannels::getNodeAddr(
    const std::string &svc_name)
{
    const auto channels = _channels(svc_name);
    return channels ? channels->selectAddr() : std::nullopt;
}

google::protobuf::Closure *ClosureFactor::create(callback_t cb)
{
    return new CallbackClosure(std::move(cb));
}

void ClosureFactor::asyncCallback(const Object::ptr &obj)
{
    std::thread([obj]()
                {
                    if (obj->callback)
                    {
                        obj->callback();
                    } })
        .detach();
}

std::shared_ptr<brpc::Server> RpcServerFactory::create(
    int port, google::protobuf::Service *svc)
{
    if (svc == nullptr)
    {
        return nullptr;
    }

    auto server = std::make_shared<brpc::Server>();
    if (server->AddService(svc, brpc::SERVER_OWNS_SERVICE) != 0)
    {
        return nullptr;
    }

    brpc::ServerOptions options;
    options.idle_timeout_sec = -1;
    if (server->Start(port, &options) != 0)
    {
        return nullptr;
    }
    return server;
}
