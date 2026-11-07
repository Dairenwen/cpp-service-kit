#pragma once
#include <json/json.h>
#include <cpr/response.h>
#include <elasticlient/client.h>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// JSON 构造器和 Elasticsearch 客户端，基于项目参考实现整理。
namespace BreezeElastic
{
    // 基础节点保存自身字段，派生类负责组织子对象或数组。
    class Base
    {
    public:
        using ptr = std::shared_ptr<Base>;
        // key 是当前节点放入父 JSON 对象时使用的名称，可以用来实现嵌套
        Base(const std::string &key);
        // 向当前节点写入普通字段；append 向指定的数组字段追加一个值。
        template <typename T>
        void add(const std::string &key, const T &val) // 普通字段直接写入
        {
            _val[key] = val;
        }
        template <typename T>
        void append(const std::string &key, const T &val) // 针对数组字段，追加一个值
        {
            _val[key].append(val);
        }
        const std::string &key() const;
        // val 返回 JSON 数据，to_string 返回可用于请求正文的 JSON 字符串。
        virtual const Json::Value val() const;
        virtual std::string to_string() const;

    protected:
        std::string _key; // 当前节点在父 JSON 对象中的名称
        Json::Value _val; // 当前节点的 JSON 数据
    };

    class Object;
    class Array : public Base // 对应数组 [ { "name": "Tom" }, { "name": "Jerry" } ]
    {
    public:
        using ptr = std::shared_ptr<Array>;
        Array(const std::string &key);
        // 数组每次创建新的子节点，因此可以容纳多个同类型查询条件
        std::shared_ptr<Object> newObject(const std::string &key);
        std::shared_ptr<Array> newArray(const std::string &key);
        void addElement(const Base::ptr &sub);
        // 清除数组节点中的全部数据
        virtual void clear();
        virtual const Json::Value val() const;
        virtual std::string to_string() const;

    private:
        std::vector<Base::ptr> _subs; // 子节点按插入顺序存储，便于组织同类型查询条件
    };

    class Object : public Base // 对应对象{ "name": "Tom" }
    {
    public:
        using ptr = std::shared_ptr<Object>;
        Object(const std::string &key);

        // 根据名称查找已有子节点
        template <typename R>
        std::shared_ptr<R> getElement(const std::string &key)
        {
            auto it = _subs.find(key);
            if (it == _subs.end())
            {
                return nullptr;
            }
            return std::dynamic_pointer_cast<R>(it->second); // 如果实际类型是 R，则返回指针，否则返回空指针
        }

        // 按名称创建或复用子对象、子数组；addElement 添加已有节点。
        std::shared_ptr<Object> newObject(const std::string &key);
        std::shared_ptr<Array> newArray(const std::string &key);
        void addElement(const Base::ptr &sub);
        // 同时清除基础字段和子节点，便于重新构造请求。
        virtual void clear();
        virtual const Json::Value val() const; // 重写 val()
        virtual std::string to_string() const; // 重写 to_string()

    private:
        std::unordered_map<std::string, Base::ptr> _subs; // 子节点按名称存储，便于复用和查找
    };

    // 自定义分析器配置：在 settings.analysis.analyzer 下设置类型和分词器。
    class Tokenizer : public Object
    {
    public:
        using ptr = std::shared_ptr<Tokenizer>;
        Tokenizer(const std::string &str);
        void tokenizer(const std::string &t);
        void type(const std::string &type);
    };

    // 下列类逐层对应 Elasticsearch 的 analyzer、analysis 和 settings 配置。
    class Analyzer : public Object
    {
    public:
        using ptr = std::shared_ptr<Analyzer>;
        Analyzer();
        Tokenizer::ptr tokenizer(const std::string &tokenizer_name);
    };
    class Analysis : public Object
    {
    public:
        using ptr = std::shared_ptr<Analysis>;
        Analysis();
        Analyzer::ptr analyzer();
    };
    class Settings : public Object
    {
    public:
        using ptr = std::shared_ptr<Settings>;
        Settings();
        Analysis::ptr analysis();
    };

    // 字段映射：设置数据类型、权重、是否建立索引以及使用的分析器。
    class Field : public Object
    {
    public:
        using ptr = std::shared_ptr<Field>;
        Field(const std::string &key);
        void type(const std::string &type);
        void boost(double boost);
        void index(bool flag);
        void analyzer(const std::string &analizer_name);
    };

    // properties 管理字段；mappings 管理字段映射及动态映射开关。
    class Properties : public Object
    {
    public:
        using ptr = std::shared_ptr<Properties>;
        Properties();
        Field::ptr field(const std::string &field_name);
    };

    class Mappings : public Object
    {
    public:
        using ptr = std::shared_ptr<Mappings>;
        Mappings();
        Properties::ptr properties();
        void dynamic(bool flag);
    };

    // 下面都是查询条件内容节点，只需要继承Object即可
    // term 是单值精确查询；terms 是同一字段的多个精确值查询。
    class Term : public Object
    {
    public:
        using ptr = std::shared_ptr<Term>;
        Term(const std::string &field);
        template <typename T>
        void setValue(const T &value)
        {
            this->add(_field, value);
        }
        const std::string &field() const { return _field; }

    private:
        std::string _field; // 需要设置的字段名
    };
    class Terms : public Object
    {
    public:
        using ptr = std::shared_ptr<Terms>;
        Terms(const std::string &field);
        template <typename T>
        void setValue(const T &value)
        {
            this->append(_field, value);
        }
        const std::string &field() const { return _field; }

    private:
        std::string _field; // 需要追加的字段名
    };
    // match 是全文查询；multi_match 可指定多个待查询的字段。
    class Match : public Object
    {
    public:
        using ptr = std::shared_ptr<Match>;
        Match(const std::string &field);
        template <typename T>
        void setValue(const T &value)
        {
            this->add(_field, value);
        }
        const std::string &field() const { return _field; }

    private:
        std::string _field; // 需要设置的字段名
    };
    class MultiMatch : public Object
    {
    public:
        using ptr = std::shared_ptr<MultiMatch>;
        MultiMatch();
        void appendField(const std::string &field);
        template <typename T>
        void setQuery(const T &query)
        {
            this->add("query", query);
        }
    };
    // range 的 setRange 设置不包含边界的 gt / lt 范围。
    class Range : public Object
    {
    public:
        using ptr = std::shared_ptr<Range>;
        Range(const std::string &field);
        template <typename T>
        void setRange(const T &gt, const T &lt)
        {
            _sub->add("gt", gt);
            _sub->add("lt", lt);
        }
        const std::string &field() const { return _sub->key(); } // 这里通过object的key来获取字段名

    private:
        Object::ptr _sub;
    };

    // 子节点按照名称进行复用，没有就进行创建
    class QObject : public Object
    {
    public:
        using ptr = std::shared_ptr<QObject>;
        QObject(const std::string &key);
        // 同一类简单查询只能绑定一个字段；不同字段请用 must/should 等数组组合。
        // 重复请求同一字段会复用节点，字段不同时抛出 std::invalid_argument。
        Term::ptr term(const std::string &field);   // 创建一个term子对象
        Terms::ptr terms(const std::string &field); // 创建一个terms子对象
        Match::ptr match(const std::string &field); // 创建一个match子对象
        MultiMatch::ptr multi_match();              // 创建一个multi_match子对象
        Range::ptr range(const std::string &field); // 创建一个range子对象
    };

    // 每次追加新的条件，必须创建一个独立的对象，避免同一字段被覆盖。
    class QArray : public Array
    {
    public:
        using ptr = std::shared_ptr<QArray>;
        QArray(const std::string &key);
        Term::ptr term(const std::string &field);
        Terms::ptr terms(const std::string &field);
        Match::ptr match(const std::string &field);
        MultiMatch::ptr multi_match();
        Range::ptr range(const std::string &field);
    };

    // must：全部满足；should：可选匹配；must_not：必须不满足。
    // must/should/must_not 都是数组，允许添加多个条件，按顺序追加，并且允许多个相同类型、不同字段，所以继承自 QArray而不是 QObject。
    class Must : public QArray
    {
    public:
        using ptr = std::shared_ptr<Must>;
        Must();
    };
    class Should : public QArray
    {
    public:
        using ptr = std::shared_ptr<Should>;
        Should();
    };
    class MustNot : public QArray
    {
    public:
        using ptr = std::shared_ptr<MustNot>;
        MustNot();
    };

    // bool 组合多个查询；minimum_should_match 设置 should 最少匹配数
    // 需要按名称复用子节点，所以需要继承 QObject 而不是 QArray
    class QBool : public QObject
    {
    public:
        using ptr = std::shared_ptr<QBool>;
        QBool();
        Must::ptr must();
        Should::ptr should();
        MustNot::ptr must_not();
        void minimum_should_match(size_t count);
    };

    // query 是搜索条件入口；match_all 表示匹配全部文档。
    class Query : public QObject
    {
    public:
        using ptr = std::shared_ptr<Query>;
        Query();
        void match_all();
        QBool::ptr qbool();
        Must::ptr must();
        Should::ptr should();
        MustNot::ptr must_not();
    };

    // 保存操作需要的索引名、类型、操作名和文档 ID。//POST http://localhost:9200/users/_doc/1001
    class Request
    {
    public:
        Request(const std::string &index, const std::string &type, const std::string &op, const std::string &id);
        void set_index(const std::string &index); // 设置索引名
        void set_type(const std::string &type);   // 设置类型名
        void set_op(const std::string &op);       // 设置操作名
        void set_id(const std::string &id);       // 设置文档 ID
        const std::string &index() const;
        const std::string &type() const;
        const std::string &op() const;
        const std::string &id() const;

    protected:
        std::string _index;
        std::string _type;
        std::string _op;
        std::string _id;
    };

    // 请求对象将索引、文档 ID 与 JSON 正文组合，供客户端发送。
    class Indexer : public Object, public Request
    {
    public:
        using ptr = std::shared_ptr<Indexer>;
        Indexer(const std::string &index);
        Settings::ptr settings();                                    // 创建或复用 settings 节点
        Tokenizer::ptr tokenizer(const std::string &tokenizer_name); // 创建或复用 tokenizer 节点
        Mappings::ptr mappings();                                    // 创建或复用 mappings 节点
        Field::ptr field(const std::string &field_name);             // 创建或复用 field 节点
    };

    // Inserter 使用 add/append 填充待插入文档，不需要成员函数，直接使用add/append即可
    class Inserter : public Object, public Request
    {
    public:
        using ptr = std::shared_ptr<Inserter>;
        Inserter(const std::string &index, const std::string &id);
    };

    // Updater 把待更新字段放进 doc 对象，表示部分更新。
    class Updater : public Object, public Request
    {
    public:
        using ptr = std::shared_ptr<Updater>;
        Updater(const std::string &index, const std::string &id);
        Object::ptr doc();
        template <typename T>
        void add(const std::string &key, const T &val)
        {
            this->doc()->add(key, val); // 直接复用已有的add函数
        }
        template <typename T>
        void append(const std::string &key, const T &val)
        {
            this->doc()->append(key, val); // 复用append进行追加
        }
    };
    // Deleter 按索引名和文档 ID 指定要删除的文档。
    class Deleter : public Object, public Request
    {
    public:
        using ptr = std::shared_ptr<Deleter>;
        Deleter(const std::string &index, const std::string &id);
    };
    // Searcher 组织 query，并通过 size/from 设置分页大小和偏移量。
    class Searcher : public Object, public Request
    {
    public:
        using ptr = std::shared_ptr<Searcher>;
        Searcher(const std::string &index);
        Query::ptr query();
        void size(size_t count);
        void from(size_t offset);
    };

    // 客户端接口及使用 elasticlient 的具体实现。
    class BaseClient
    {
    public:
        using ptr = std::shared_ptr<BaseClient>;
        BaseClient() = default;
        virtual ~BaseClient() = default;
        virtual bool create(const Indexer &idx) = 0;       // 创建索引
        virtual bool insert(const Inserter &ins) = 0;      // 插入数据
        virtual bool update(const Updater &upd) = 0;       // 更新数据
        virtual bool remove(const Deleter &del) = 0;       // 删除数据
        virtual bool remove(const std::string &index) = 0; // 删除索引
        // 成功时返回命中文档的 _source 数组（无命中为空数组）；失败时返回空 optional。
        virtual std::optional<Json::Value> search(const Searcher &sea) = 0;
    };

    class ESClient : public BaseClient
    {
    public:
        using ptr = std::shared_ptr<ESClient>;
        ESClient(const std::vector<std::string> &hosts);
        bool create(const Indexer &idx) override;
        bool insert(const Inserter &ins) override;
        bool update(const Updater &upd) override;
        bool remove(const Deleter &del) override;
        bool remove(const std::string &index) override;
        std::optional<Json::Value> search(const Searcher &sea) override;

    private:
        std::shared_ptr<elasticlient::Client> _client;
    };

    // 工厂返回 BaseClient 接口，调用方可使用该接口执行各类操作。
    class ESClientFactory
    {
    public:
        static BaseClient::ptr create(const std::vector<std::string> &hosts); // 可以填写多个 Elasticsearch 节点的 URL，逗号分隔。
    };
}
