# AMQP-SDK

AMQP，即Advanced Message Queuing Protocol，一个提供统一消息服务的应用层标准高级消息队列协议，是应用层协议的一个开放标准，为面向消息的中间件设计。基于此协议的客户端与消息中间件可传递消息，并不受客户端/中间件不同产品，不同的开发语言等条件的限制。Erlang中的实现有RabbitMQ等。

它规定了消息系统中三大组件——消息服务器/代理节点(server/broker)、生产者/发布者(producer/publisher)、消费者/订阅者(consumer/subscriber)之间的通信规范，以及代理节点的设计规范等。

### MQ核心要素

1. Broker：消息代理服务器（比如RabbitMQ）
2. 虚拟机（Virtual Host）：虚拟主机，隔离环境
3. 交换机（Exchange）：
   - 交换机是消息的分发中心，它接收生产者发送的消息并根据一定的规则将消息路由到一个或多个队列中。比较典型的交换机类型，包括：
     - 直连交换机（Direct Exchange）：根据消息的路由键将消息发送到特定队列。
     - 主题交换机（Topic Exchange）：根据消息的路由键和通配符匹配将消息发送到多个队列。
     - 广播交换机（Fanout Exchange）：将消息广播到与交换机绑定的所有队列。
     - 头交换机（Headers Exchange）：根据消息的自定义头部属性进行匹配路由。
4. 队列（Queue）：
   - 队列是消息的容器，它存储消息直到消费者准备好接收和处理它们。消息通过交换机路由到队列，消费者可以从队列中读取消息。每个队列都有一个名称，它们可以绑定到一个或多个交换机，并指定了消息的路由规则。
5. Binding-Key：
   - Binding-Key是交换机和队列之间的绑定键，用于定义消息的路由规则。在直连交换机中，Binding-Key通常与队列的路由键一致；在主题交换机中，Binding-Key可以使用通配符进行匹配。
6. 生产者（Producer）：
   - 生产者是消息的发送方。它们创建消息并将其发布到RabbitMQ的交换机上。生产者通常将消息发送到一个或多个队列，以便消费者可以订阅并处理这些消息。
7. 消费者（Consumer）：
   - 消费者是消息的接收方，它们订阅队列并从中获取消息。一旦消费者接收到消息，它们可以对消息进行处理，例如执行某些任务或将数据存储到数据库中。

### 工作流程

消息的传输流程：

- 生产者连接Broker进行发布消息，并指定交换机和路由键。
- 交换机根据路由键将消息路由到一个或多个队列。
- 消费者从队列中获取消息并处理它们

```mermaid

flowchart LR
    P["Producer<br/>生产者"] -->|"发送消息<br/>Routing-Key"| E

    subgraph B["Broker（消息代理服务器 / RabbitMQ）"]
        subgraph V["Virtual Host（虚拟主机 / 隔离环境）"]
            E["Exchange<br/>交换机"]
            K["Binding-Key<br/>绑定键 / 路由规则"]
            Q1[("Queue 1<br/>队列")]
            Q2[("Queue 2<br/>队列")]
            Q3[("Queue 3<br/>队列")]

            E -->|"路由消息"| Q1
            E -->|"路由消息"| Q2
            E -->|"路由消息"| Q3
            K -.->|"定义绑定规则"| E
            K -.-> Q1
            K -.-> Q2
            K -.-> Q3
        end
    end

    Q1 --> C1["Consumer 1<br/>消费者"]
    Q2 --> C2["Consumer 2<br/>消费者"]
    Q3 --> C3["Consumer 3<br/>消费者"]

    classDef producer fill:#dbeafe,stroke:#3b82f6,color:#1e40af
    classDef exchange fill:#fef3c7,stroke:#d97706,color:#92400e
    classDef queue fill:#dcfce7,stroke:#16a34a,color:#166534
    classDef consumer fill:#dbeafe,stroke:#3b82f6,color:#1e40af
    classDef binding fill:#f3e8ff,stroke:#9333ea,color:#6b21a8

    class P producer
    class E exchange
    class Q1,Q2,Q3 queue
    class C1,C2,C3 consumer
    class K binding

```

### 作用

1. 解耦：消息队列可以将消息的生产者和消费者解耦，它们不需要同时在线或直接交互，只需要通过队列进行通信。
2. 异步处理：消息队列允许异步处理任务，这意味着生产者可以发送消息而不必等待消费者处理消息的结果，从而提高性能和吞吐量。
3. 削峰填谷：将短时间高并发产生的事务消息存储在消息队列中，然后后端服务再慢慢根据自己的能力去消费这些消息，这样就避免直接把后端服务打垮掉。
4. 负载均衡：在多个消费者的情况下，消息队列可以自动分配消息到不同的消费者，实现负载均衡。
5. 灵活性和可扩展性：消息队列支持水平扩展，可以根据需要添加更多的消费者来处理消息。
6. 增强系统的可维护性：通过将消息传递机制与业务逻辑分离，系统的不同部分可以独立开发和维护。
7. 容错性：消费者处理消息失败时，消息队列可以重新路由消息，或者将其放入死信队列以便后续处理。
8. 支持复杂的业务流程：消息队列可以支持复杂的业务流程，如工作流和任务调度。
9. 数据分发：消息队列可以用于数据分发，将数据从一个地方传输到另一个地方，特别是在分布式系统中。
10. 日志记录：消息队列可以用于日志记录，收集来自不同源的日志信息，然后进行集中处理。
11. 跨语言和平台：消息队列通常支持多种编程语言和平台，使得不同技术栈的系统可以轻松集成。

## AMQP-CPP简介

AMQP-CPP是一个用于与RabbitMQ消息中间件通信的C++库。以下是AMQP-CPP的一些关键特性和介绍：

1. 项目简介：AMQP-CPP是一个强大的开源库，实现了Advanced Message Queuing Protocol（AMQP）的客户端，允许使用C++轻松地与支持AMQP的消息中间件如RabbitMQ、Qpid等进行交互。该项目由Copernica Marketing Software开发并维护，旨在提供高效、稳定且易于使用的AMQP接口。
2. 核心特性：
   - 异步非阻塞通信：AMQP-CPP完全异步，没有阻塞式的系统调用，不使用线程就能够应用在高性能应用中。这使得它非常适合需要高吞吐量和快速响应的应用程序。
   - 分层架构：AMQP-CPP具有分层架构，允许用户按需实现网络层。这意味着用户可以自己管理网络连接和IO操作，或者使用AMQP-CPP提供的预定义TCP和TLS模块。
   - 跨平台兼容性：AMQP-CPP支持Linux、macOS、Windows等多种操作系统，不依赖特定操作系统调用，易于集成进各种事件循环。
   - C++17特性支持：充分利用现代C++标准的功能，要求编译器需支持C++17或以上版本，保证代码的先进性和效率。
3. 安装和使用：
   - AMQP-CPP支持两种编译方式：make和cmake。Linux支持静态库和共享库，而Windows上目前不支持共享库。在Windows上编译时需要定义NOMINMAX 宏。
   - 有两个头文件需要使用：amqpcpp.h （总是需要的）和amqpcpp/linux_tcp.h （仅在使用Linux版本的TCP模块时需要）。
4. 应用场景：AMQP-CPP广泛应用于需要高性能消息传递的场景，例如大规模、低延迟的消息处理环境。
5. 技术实践：AMQP-CPP的核心在于其对AMQP协议的高度抽象封装。它将复杂的网络通信细节隐藏于简洁易用的API之后。通过定义一系列回调函数（如onData , onError , onReady ），开发者可以轻松管理与RabbitMQ之间的连接状态与数据流。
6. 项目地址：https://github.com/CopernicaMarketingSoftware/AMQP-CPP

### 安装

```bash
git clone https://github.com/CopernicaMarketingSoftware/AMQP-CPP.git
cd AMQP-CPP/
make && sudo make install
```

### 接口

AMQP-CPP的使用有两种模式：

- 使用默认的TCP模块进行网络通信
- 使用扩展的libevent、libev、libuv、asio异步网络通信组件进行通信

#### 默认TCP模式

- 实现一个类继承自AMQP::TcpHandler类， 它负责网络层的TCP连接
- 重写相关函数， 其中必须重写monitor函数
- 在monitor函数中需要实现的是将fd放入eventloop(select、epoll)中监控， 当fd可写可读就绪之后， 调用AMQP-CPP的connection->process(fd, flags)方法

```cpp
#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

class MyTcpHandler : public AMQP::TcpHandler
{
    /**
        *AMQP库在创建新连接时调用的方法
        *与处理程序相关联。这是对处理程序的第一次调用
        *@param connection附加到处理程序的连接
    */
    virtual void onAttached(AMQP::TcpConnection *connection) override
    {
        //@todo
        //添加您自己的实现，例如初始化事物
        //以处理连接。
    }

    /**
        *当TCP连接时由AMQP库调用的方法
        *已经建立。调用此方法后，库
        *仍然需要设置可选的TLS层和
        *在TCP层的顶部建立AMQP连接。，这种方法
        *总是与稍后对onLost（）的调用配对。
        *@param connection现在可以使用的连接
    */
    virtual void onConnected(AMQP::TcpConnection *connection) override
    {
        //@todo
        //添加您自己的实现（可能不需要）
    }

    /**
        *在建立安全TLS连接时调用的方法。
        *这只对amqps://连接调用。它允许您检查连接是否足够安全，以满足您的喜好
        *（例如，您可以检查服务器证书）。AMQP协议仍然需要启动。
        *@param connection已被保护的连接
        *@param ssl来自openssl库的ssl结构
        *@return bool如果可以使用连接，则为True
    */
    virtual bool onSecured(AMQP::TcpConnection *connection, const SSL *ssl) override
    {
        //@todo
        //添加您自己的实现，例如读取证书并检查它是否确实是您的
        return true;
    }
    /**
        *当登录尝试成功时由AMQP库调用的方法。在此之后，连接就可以使用了。
        *@param connection现在可以使用的连接
    */
    virtual void onReady(AMQP::TcpConnection *connection) override
    {
        //@todo
        //添加您自己的实现，例如通过创建一个通道实例，然后开始发布或使用
    }
   /**
       *该方法在服务器尝试协商检测信号间隔时调用，
       *并被覆盖以摆脱默认实现（否决建议的检测信号间隔），转而接受该间隔。
       *@param connection发生错误的连接
       *@param interval建议的间隔（秒）
   */
   virtual uint16_t onNegotiate(AMQP::TcpConnection *connection, uint16_t interval)
   {
       //我们接受服务器的建议，但如果间隔小于一分钟，我们将使用一分钟的间隔
       if (interval < 60) interval = 60;

       //@todo
       //在事件循环中设置一个计时器，
       //如果在这段时间内没有发送其他指令，
       //请确保每隔interval秒调用connection->heartbeat（）。
       //返回我们要使用的间隔
       return interval;
   }
   /**
       *发生致命错误时由AMQP库调用的方法
       例如，因为无法识别从RabbitMQ接收的数据，或者基础连接丢失。
       此调用之后通常会调用onLost（）（如果错误发生在TCP连接建立之后）和onDetached（）。
       *@param connection发生错误的连接
       *@param message一条人类可读的错误消息
   */
   virtual void onError(AMQP::TcpConnection *connection, const char *message) override
   {
       //@todo
       //添加您自己的实现，例如，通过向程序的用户报告错误并记录错误
   }
   /**
       *该方法在AMQP协议结束时调用的方法。这是调用connection.close（）以正常关闭连接的计数器部分。请注意，TCP连接此时仍处于活动状态，您还将收到对onLost（）和onDetached（）的调用
       @param connection AMQP协议结束的连接
   */
   virtual void onClosed(AMQP::TcpConnection *connection) override
   {
       //@todo
       //添加您自己的实现, 可能没有必要，
       //但如果您想在amqp连接结束后立即执行某些操作，
       //又不想等待tcp连接关闭，则这可能会很有用
   }
   /**
       *当TCP连接关闭或丢失时调用的方法。
       *如果同时调用了onConnected（），则始终调用此方法
       *@param connection已关闭但现在无法使用的连接
   */
   virtual void onLost(AMQP::TcpConnection *connection) override
   {
       //@todo
       //添加您自己的实现（可能没有必要）
   }
   /**
       *调用的最终方法。这表示将不再对处理程序进行有关连接的进一步调用。
       *@param connection可以被破坏的连接
   */
   virtual void onDetached(AMQP::TcpConnection *connection) override
   {
       //@todo
       //添加您自己的实现，如清理资源或退出应用程序
   }

  /**
   *当AMQP-CPP库想要与主事件循环交互时，它会调用该方法。
   *AMQP-CPP库是完全不阻塞的，
   *并且只有在事先知道这些调用不会阻塞时才进行“write()”或“read()”系统调用。
   *要在事件循环中注册文件描述符，它会调用这个“monitor()”方法，
   *该方法带有一个文件描述符和指示是否该检查文件描述符的可读性或可写性的标志。
   *
   *@param connection想要与事件循环交互的连接
   *@param fd应该检查的文件描述符
   *@param标记位或AMQP:：可读和/或AMQP：：可写
   */
   virtual void monitor(AMQP::TcpConnection *connection, int fd, int flags) override
   {
       //@todo
       //添加您自己的实现，
       //例如将文件描述符添加到主应用程序事件循环(如select()或poll()循环)。
       //当事件循环报告描述符变为可读和或可写时，
       //由您通过调用connection->process(fd，flags)方法
       //通知AMQP-CPP库文件描述符处于活动状态。
   }
};
```

#### 扩展模式

以libev为例， 我们不必要自己实现monitor函数， 可以直接使用AMQP::LibEvHandler

##### 头文件包含

```cpp
#include <amqpcpp.h>
```

##### Connection

用于设定rabbitmq服务器地址后，创建一个通信连接对象。

```cpp

class LibEvHandler : public TcpHandler;
class ConnectionHandler;
class Address {
   // amqp://user:pass@host:port/
   Address(const std::string &address)gg
}
class TcpConnection : private ConnectionHandler {
   TcpConnection(TcpHandler *handler, const Address &address)
}
class Login
{
   Login(std::string user, std::string password)
}
class Connection
{
   Connection(
       ConnectionHandler *handler,
       const Login &login,
       const std::string &vhost)
   bool close()//关闭连接
}
```

##### Channel

channel是一个虚拟连接，一个连接上可以建立多个通道。并且所有的RabbitMq指令都是通过channel传输，所以连接建立后的第一步，就是建立channel。因为所有操作是异步的，所以在channel上执行指令的返回值并不能作为操作执行结果，实际上它返回的是 Deferred 类，可以使用它安装处理函数。

```cpp
namespace AMQP {
/**
*  通用回调函数类型
*/
using SuccessCallback = std::function<void()>;
using ErrorCallback = std::function<void(const char *message)>;
using FinalizeCallback = std::function<void()>;

//队列的声明的成功回调函数
using QueueCallback = std::function<void(
   const std::string &name,
    uint32_t messagecount,
    uint32_t consumercount)>;
//队列删除回调函数
using DeleteCallback = std::function<void(uint32_t deletedmessages)>;

//消息处理回调函数
using MessageCallback = std::function<void(
   const Message &message,  //收到的消息
   uint64_t deliveryTag,    //消息的唯一标识
   bool redelivered)>;      //指示消息是否是被重新投递的

//当启用发布者确认时，当服务器确认消息已被接收和处理时，将调用AckCallback
using AckCallback = std::function<void(
   uint64_t deliveryTag,     //消息的唯一标识符
   bool multiple)>;    //指示确认范围：false确认单个消息，true确认所有消费的该消息

//消息被RabbitMQ成功接收和处理后调用的回调函数
using PublishAckCallback  = std::function<void()>;
//消息被RabbitMQ明确拒绝（nacked）时调用的回调函数
using PublishNackCallback = std::function<void()>;
//消息丢失时调用的回调函数
using PublishLostCallback = std::function<void()>;
}
```

这些回调函数使得AMQP-CPP库能够在消息发布后提供反馈，帮助应用程序实现更可靠的消息传递机制。通过实现这些回调，开发者可以对消息的发布结果进行适当的处理，确保消息传递的可靠性或进行错误恢复

```cpp
extern const int durable;
extern const int autodelete;
extern const int active;
extern const int passive;
extern const int ifunused;
extern const int ifempty;
extern const int global;
extern const int nolocal;
extern const int noack;
extern const int exclusive;
extern const int nowait;
extern const int mandatory;
extern const int immediate;
extern const int redelivered;
extern const int multiple;
extern const int requeue;
extern const int readable;
extern const int writable;
extern const int internal;
namespace AMQP {
enum ExchangeType
{
    fanout,  //广播交换，绑定的队列都能拿到消息
    direct,  //直接交换，只将消息交给routingkey一致的队列
    topic,   //主题交换，将消息交给符合bindingkey规则的队列
    headers,
    consistent_hash,
    message_deduplication
};
```

```cpp
class Channel {
    Channel(Connection *connection);
    bool connected();//是否连接可用
    Deferred &close();//关闭当前channel
    DeferredConfirm &confirmSelect();
    Deferred &startTransaction();
    Deferred &commitTransaction();
    Deferred &rollbackTransaction();
    /**
        *声明交换机
        *以下flags可用于交换机：
            *-durable     持久化，重启后交换机依然有效
            *-autodelete  删除所有连接的队列后，自动删除交换
            *-passive     仅被动检查交换机是否存在
            *-internal    创建内部交换
        *此函数返回一个延迟处理程序。可以安装回调
        using onSuccess(), onError() and onFinalize() methods.
    */
    Deferred &declareExchange(
        const std::string_view &name, //交换机名称
        ExchangeType type,            //交换机类型
        int flags)                    //标志位
    /**
        *声明队列
        *flags可以是以下值的组合：
            *-durable 持久队列在代理重新启动后仍然有效
            *-autodelete 当所有连接的使用者都离开时，自动删除队列
            *-passive 仅被动检查队列是否存在
            *-exclusive 队列仅存在于此连接，并且在连接断开时自动删除
        *可安装回调：onSuccess（）、onError（）和onFinalize（）方法。
        Deferred &onError(const char *message)
        Deferred &onSuccess(QueueCallback&)
            void myCallback(const std::string &name,
                uint32_t messageCount,
                uint32_t consumerCount);
        例如：channel.declareQueue("myqueue").onSuccess(myCallback);
    */
    DeferredQueue &declareQueue(
        const std::string_view &name, //队列名称
        int flags)                    //队列标志位
    /**
        *将队列绑定到交换机
        *可以安装回调：onSuccess（）、onError（）和onFinalize（）方法。
    */
    Deferred &bindQueue(
        const std::string_view &exchange, //交换机名称
        const std::string_view &queue,    //队列名称
        const std::string_view &bindingkey) //绑定Key)
    Deferred &unbindQueue(
        const std::string_view &exchange,
        const std::string_view &queue,
        const std::string_view &bindingkey)
   /*
       *删除队列
       *flag
           -   ifunused    仅在队列没有连接者时删除
           -   ifempty     仅在队列为空时删除
   */
    DeferredDelete &removeQueue(
        const std::string_view &name,
        int flags = 0)
    /**
        *发布消息
        flags参数指定如果消息无法路由到队列时应该发生的情况。
        默认情况下，不可更改的消息将被静默地丢弃。
        发布之前，请确保您已经调用了recall()方法，
        并设置了所有适当的处理程序来处理这些返回的消息。
        *flags：
            *-mandatory 如果设置，服务器将返回未发送到队列的消息
            *-immediate 如果设置，服务器将返回无法立即转发给使用者的消息。
    */
   bool publish(
       const std::string_view &exchange,  //交换机名称
       const std::string_view &routingKey,//路由Key
       const std::string &message,        //消息内容
       int flags = 0)                     //标志位
   /*
       告诉消息您已准备好召回/收回无法转发的消息
       当你将publish()方法与“immediate”或“mandatory”标志结合使用时，
       rabbitmq会发回不可路由的消息。
       使用此recall()方法，您可以安装一种伪消费者，定义如何处理此类返回的消息
   */
   DeferredRecall &recall();
   /**
       *订阅队列消息
       消费者标识用于通过channel::cancel()调用取消订阅
       *支持以下flags：
           *-nolocal    如果设置了，则不会同时消耗在此通道上发布的消息
           *-noack      如果设置了，则不必对已消费的消息进行确认
           *-exclusive  请求独占访问，只有此使用者可以访问队列
       可以安装回调：onSuccess（）、onError（）和onFinalize（）方法。
       可以安装的onSuccess（）回调应该具有以下格式：
           void myCallback（const std:：string_view&tag）；
               tag：即订阅时传入的消费者标识
   */
   DeferredConsumer &consume(
       const std::string_view &queue, //队列名称
       const std::string_view &tag,   //消费者标识
       int flags = 0)                     //标志位
   /*
       取消订阅
       flags：
           -   noack 消费消息不需要确认，也就是自动确认
   */
   DeferredCancel &cancel(const std::string_view &tag);
   /*
       从MQ检索单条消息
   */
   DeferredGet &get(const std::string_view &queue, int flags = 0);
   /**
       *消费者确认接收到的消息
       *当在DeferredConsumer::onReceived()方法中接收到消息时，必须确认该消息，
       以便RabbitMQ将其从队列中删除（除非使用noack选项消费）。
       flag:
           multiple 确认多条消息：之前传递的所有未确认的消息也会被确认
   */
   bool ack(uint64_t deliveryTag, int flags=0)
   /*
       消费者拒绝消息
       flags:
           multiple:拒绝多条消息：之前传递的所有未确认的消息也都是未确认的
           requeue:如果已设置，则消息将放回队列中，否则将被删除
   */
   bool reject(uint64_t deliveryTag, int flags=0)

   //回复所有还没有被确认的消息
   //flags:requeue:如果设置了，服务器将重新对消息进行排队，因此也可能最终到达不同的消费者
   Deferred &recover(int flags = 0);
}
}
```

##### Reliable

用于可靠的消息发布及错误处理。

```cpp
template <typename BASE=Tagger>
class Reliable : public BASE
{
   template <typename ...Args>
   Reliable(Args &&...args)
   DeferredPublish &publish(
       const std::string_view &exchange,
       const std::string_view &routingKey,
       const std::string_view &message,
       int flags = 0)
}

//样例：
AMQP::TcpChannel mychannel(connection);
AMQP::Reliable reliable(mychannel);
reliable.publish("my-exchange", "my-key", "my first message")
   .onAck()
   .onNack()
   .onLost()
   .onError();
```

##### Message

用于订阅者收到消息后的处理

```cpp

class Envelope : public MetaData{
   const char *body()
   uint64_t bodySize()
}
class Message : public Envelope{
   const std::string &exchange()
   const std::string &routingkey()
}
```

##### Deferred

Deferred  类（包括其派生类，如 DeferredQueue , DeferredConsumer  等）是处理异步操作的核心机制。它用于表示一个尚未完成的操作，用于设置异步调用的回调函数，进行接口的连续调用。

```cpp
class Deferred
{
   Deferred &onSuccess(const SuccessCallback& callback)
   Deferred &onError(const ErrorCallback& callback)
   Deferred &onFinalize(const FinalizeCallback& callback)
}
```

示例：声明交换机

```cpp
//channel.declareExchange() 接口返回的是Deferred对象
//因此，可以连续调用onSuccess设置成功处理回调函数以及onError失败处理回调函数
channel.declareExchange(exchange, AMQP::ExchangeType::direct)
    .onSuccess([&](){
        std::cout << "声明交换机成功:" << exchange << std::endl;
    })
    .onError([&](const char *message) {
        std::cout << "声明交换机失败:" << message << std::endl;
    });
```

##### DeferredQueue

Channel中队列相关操作接口的返回对象，用于设置队列操作的相关回调函数。

```cpp
class DeferredQueue : public Deferred
{
   DeferredQueue &onSuccess(const QueueCallback& callback)
   DeferredQueue &onSuccess(const SuccessCallback& callback)
}
using QueueCallback = std::function<void(const std::string &name,
               uint32_t messageCount,
               uint32_t consumerCount)>;
```

示例：声明队列

```cpp
//channel.declareQueue() 接口返回的是DeferredQueue对象
channel.declareQueue(queue)
    .onSuccess([&](const std::string &name,
         uint32_t messagecount,
         uint32_t consumercount){
        std::cout << "声明队列成功:" << queue << std::endl;
        std::cout << "队列中有" << messagecount << "个消息" << std::endl;
        std::cout << "队列中有" << consumercount << "个消费者" << std::endl;
    })
    .onError([&](const char *message) {
        std::cout << "声明队列失败:" << message << std::endl;
        abort(); // 终止程序
    });
```

##### DeferredConsumer

Channel中订阅相关操作接口的返回对象，用于设置订阅的相关回调函数。

```cpp
class DeferredConsumer :  public Deferred {
   DeferredConsumer &onSuccess(const ConsumeCallback& callback)
   //收到消息的回调函数
   DeferredConsumer &onReceived(const MessageCallback& callback)
   /* Alias for onReceived() */
   DeferredConsumer &onMessage(const MessageCallback& callback)
   //取消订阅的回调
   DeferredConsumer &onCancelled(const CancelCallback& callback)
}
using ConsumeCallback = std::function<void(const std:：string_view &tag)>；
using MessageCallback = std::function<void (const AMQP::Message &message,
       uint64_t deliveryTag,
       bool redelivered)>;
```

示例：订阅队列消息

```cpp
//channel.consume() 接口返回的是DeferredConsumer对象
channel.consume(queue)
    .onMessage([&](const AMQP::Message &message,  //收到的消息
        uint64_t deliveryTag,    //消息的唯一标识
        bool redelivered){
        std::string body(message.body(), message.bodySize());
        std::cout << "收到消息:" << body << std::endl;
        // 7.5 收到消息进行处理后，不要忘了对消息进行确认
        channel.ack(deliveryTag);
    })
    .onError([&](const char *message) {
        std::cout << "订阅队列消息失败:" << message << std::endl;
        abort(); // 终止程序
    })
    .onSuccess([&](const std:：string_view &tag){
        std::cout << "订阅队列消息成功:" << std::endl;
    });
```

#### 扩展网络库libev

##### LibEvHandler

amqpcpp库与libev网络库的对接

```cpp
#include <amqpcpp/libev.h>
#include <ev.h>
class LibEvHandler : public TcpHandler{
   LibEvHandler(struct ev_loop *loop, int priority = 0)
}
```

##### ev_loop

libev网络通信库的核心使用接口。

```cpp
typedef struct ev_async
{
 EV_WATCHER (ev_async)

 EV_ATOMIC_T sent; /* private */
} ev_async;

//break type
enum {
 EVBREAK_CANCEL = 0, /* undo unloop */
 EVBREAK_ONE    = 1, /* unloop once */
 EVBREAK_ALL    = 2  /* unloop all loops */
};

//创建libev操作句柄
struct ev_loop *ev_default_loop (unsigned int flags EV_CPP (= 0))
# define EV_DEFAULT  ev_default_loop (0)
void ev_loop_destroy (struct ev_loop *loop)

//开始事件循环 --- 阻塞接口
int  ev_run (struct ev_loop *loop);
//退出事件循环
void ev_break (struct ev_loop *loop, int32_t break_type) ;

//异步事件回调函数
//也就是每个外部针对ev的操作都必须在evloop所在线程内执行
//这里相当打包了一个任务，加入到evloop的任务队列中进行执行
void (*callback)(struct ev_loop *loop, ev_async *watcher, int32_t revents)
//初始化一个异步任务
void ev_async_init(ev_async *w, callback cb);
//启动loop的异步任务监测，这样就可以接收任务进行处理了
void ev_async_start(struct ev_loop *loop, ev_async *w) ;
//发送异步任务到loop中进行执行
void ev_async_send(struct ev_loop *loop, ev_async *w) ;

//示例：异步退出
void mycb(struct ev_loop *loop, ev_async *watcher, int32_t revents) {
   ev_break(loop, EVBREAK_ALL);
}
ev_async ea;
ev_async_init(&ea, mycb);
ev_async_start(loop, &ea);
ev_async_send(loop, &ea);
```

#### 死信队列

##### 要素：

1. 死信消息 (Dead Letter Message): 当一个消息满足某些特定条件时（消息拒绝，消息过期，队列满了），它就会变成“死信消息”。
2. 死信交换机 (Dead Letter Exchange - DLX): 这是一个普通的交换机。当一个消息变成死信后，RabbitMQ 会将其重新发布（publish ）到这个特定的交换机。
3. 死信队列 (Dead Letter Queue - DLQ): 这是一个绑定到死信交换机上的普通队列。死信交换机将死信消息路由（通过绑定的 Routing Key）到这个队列进行存储。应用程序可以消费这个队列中的消息来处理这些“死信”。

##### 使用流程：

1. 创建普通交换机A和普通队列A并进行绑定（作为死信交换机&队列存在）
2. 创建普通交换机B和普通队列B并进行绑定（作为常规交换机&队列存在）

a. 创建队列B的时候设置关联的死信交换机相关信息

##### 相关接口与类

```cpp
class Field {
}
class Table : public Field {
    public:
        Table &set(const std::string &name, bool value) ;
        // ... 一系列常见基础类型的重载...
        AssociativeFieldProxy operator[](const std::string& name)
}

//死信队列参数
AMQP::Table args;
args["x-dead-letter-exchange"] = "xxx";
args["x-dead-letter-routing-key"] = "xxx";
args["x-message-ttl"] = 10000; //队列消息生命周期设置-以毫秒为单位
```

## 封装

### 设计

消息队列的操作基本五个操作：声明交换机，声明队列，绑定交换机-队列，发布消息，订阅队列消息。

而基于这五个操作，封装一些类出来进行简化：

- 套件配置声明结构：便于交换机与队列的声明与绑定，以及队列的订阅和消息的发布
- 消息队列客户端类：将客户端的各个请求操作封装起来，简化外部操作
- 发布者类：基于消息队列客户端类进行上层封装，用于发布端进行消息发布
- 订阅者类：基于消息队列客户端类进行上层封装，用于订阅端进行消息订阅

#### 套件配置声明结构体

##### 成员

```markdown
- 交换机名称
- 交换机类型
- 队列名称
- 绑定关键字
- 延迟时间
```

##### 方法

```markdown
- 获取死信交换机名称
- 获取死信队列名称
- 获取死信队列BindingKey通过结构体中的信息，可以用于进行交换机与队列的声明与绑定，也可以用于向队列发布消息，还可以用于订阅队列消息。
```

#### 消息队列客户端类

通过实例化的对象，让使用者能够快速便捷进行消息操作。

##### 成员变量：

    - 互斥锁
    - 条件变量
    - libev事件触发器句柄
    - 连接对象句柄
    - 信道对象句柄
    - 异步监视器句柄
    - 异步工作线程

##### 成员方法：

    - 声明交换机与队列并进行绑定
        - 声明交换机以及队列并进行绑定后，需要等待实际交换机与队列声明成功
        - 在绑定成功回调函数中，进行通知唤醒
    - 发布消息
    - 订阅队列消息
    - 启动事件循环（因为我们是使用异步线程进行事件循环，因此构造成员成功后就直接启动）
    - 等待结束

#### 发布者类

基于消息队列客户端类所实例化的对象，进行二次封装，实现一个能够便捷进行消息发布的类。

##### 成员变量：

    - 消息队列客户端对象
    - 套件配置声明对象

##### 成员方法：

    - 发布消息

#### 订阅者类

基于消息队列客户端类所实例化的对象，进行二次封装，实现一个能够便捷进行消息订阅的类。

##### 成员变量：

    - 消息队列客户端对象
    - 套件声明配置对象

##### 成员方法：

    - 订阅队列消息（如果订阅的队列是一个延时队列，则实际上订阅的是对应关联的死信队列）

**封装后的结构如下：**

```mermaid
flowchart TB
    Config["declare_settings 配置<br/>交换机、队列、路由键、延迟时间"]

    Publisher["Publisher 发布者<br/>负责发送消息"]
    Subscriber["Subscriber 订阅者<br/>负责接收消息并调用业务回调"]

    Config --> Publisher
    Config --> Subscriber

    Publisher --> Client
    Subscriber --> Client

    subgraph Client["MQClient 消息队列客户端"]
        Channel["Channel 通道<br/>声明、发布、订阅、确认消息"]
        Connection["Connection 连接<br/>负责与 RabbitMQ 通信"]
        Loop["libev 事件循环<br/>处理网络事件和回调"]

        Channel --> Connection
        Loop -.处理事件.-> Connection
    end

    Connection <--> RabbitMQ["RabbitMQ 服务器<br/>管理交换机和队列"]

    Factory["MQFactory 工厂<br/>创建客户端、发布者和订阅者"]
    Factory -.创建.-> Publisher
    Factory -.创建.-> Subscriber
    Factory -.创建.-> Client
```
