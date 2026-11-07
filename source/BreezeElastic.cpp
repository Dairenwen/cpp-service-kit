#include "BreezeElastic.hpp"
#include "BreezeUtil.hpp"
#include "BreezeLog.hpp"
#include <optional>
#include <stdexcept>

namespace BreezeElastic
{

    // 基础节点与对象/数组节点的 JSON 序列化。
    Base::Base(const std::string &key) : _key(key) {}
    const std::string &Base::key() const { return _key; }
    const Json::Value Base::val() const { return _val; }
    std::string Base::to_string() const
    {
        Json::Value root;
        root[_key] = _val;
        return *BreezeUtil::JSON::serialize(root, true); // 开启缩进，进行序列化
    }

    Array::Array(const std::string &key) : Base(key) {}
    std::shared_ptr<Object> Array::newObject(const std::string &key)
    {
        auto obj = std::make_shared<Object>(key);
        this->addElement(obj);
        return obj;
    }
    std::shared_ptr<Array> Array::newArray(const std::string &key)
    {
        auto arr = std::make_shared<Array>(key);
        this->addElement(arr);
        return arr;
    }
    void Array::addElement(const Base::ptr &sub)
    {
        _subs.push_back(sub);
    }
    void Array::clear()
    {
        _subs.clear();
        _val = Json::Value();
    }
    const Json::Value Array::val() const
    {
        // 按插入顺序把每个子节点的 JSON 值加入数组。
        Json::Value root = _val;
        for (auto &i : _subs)
        {
            root.append(i->val());
        }
        return root;
    }
    std::string Array::to_string() const
    {
        Json::Value root = _val;
        for (auto &i : _subs)
        {
            root.append(i->val());
        }
        return *BreezeUtil::JSON::serialize(root, true);
    }

    Object::Object(const std::string &key) : Base(key) {}
    std::shared_ptr<Object> Object::newObject(const std::string &key)
    {
        // 对象以名称区分子节点：已有同名节点时直接复用。
        Object::ptr sub;
        sub = this->getElement<Object>(key);
        if (sub)
        {
            return sub;
        }
        sub = std::make_shared<Object>(key);
        this->addElement(sub);
        return sub;
    }
    std::shared_ptr<Array> Object::newArray(const std::string &key)
    {
        Array::ptr array;
        array = this->getElement<Array>(key);
        if (array)
        {
            return array;
        }
        array = std::make_shared<Array>(key);
        this->addElement(array);
        return array;
    }
    void Object::addElement(const Base::ptr &sub)
    {
        _subs[sub->key()] = sub;
    }
    void Object::clear()
    {
        // 既清除子节点，也清除 add 写入的字段
        _subs.clear();
        _val = Json::Value();
    }
    const Json::Value Object::val() const
    {
        Json::Value root = _val; // 将当前val的数据赋值给root
        for (auto &i : _subs)
        { // 当前对象也可能有自己子对象，把子对象的val拿出来赋值到root
            root[i.first] = i.second->val();
        }
        return root;
    }
    std::string Object::to_string() const
    {
        Json::Value root = _val; // 先拿本身的数据
        for (auto &i : _subs)
        { // 再拿自己的子对象的数据
            root[i.first] = i.second->val();
        }
        return *BreezeUtil::JSON::serialize(root, true);
    }

    // 组装索引设置、分词器和字段映射。
    Tokenizer::Tokenizer(const std::string &str) : Object(str)
    {
        // 默认使用 IK 最大词粒度分词器，可由调用方覆盖。
        this->tokenizer("ik_max_word");
        this->type("custom"); // 默认使用自定义分析器类型
    }
    void Tokenizer::tokenizer(const std::string &t)
    {
        this->add("tokenizer", t); // 设置分词器
    }
    void Tokenizer::type(const std::string &type)
    {
        this->add("type", type); // 设置分析器类型
    }

    Analyzer::Analyzer() : Object("analyzer") {} // 用analyzer固定值来创建分词器配置
    Tokenizer::ptr Analyzer::tokenizer(const std::string &tokenizer_name)
    {
        Tokenizer::ptr tokenizer;
        tokenizer = this->getElement<Tokenizer>(tokenizer_name);
        if (tokenizer)
        {
            return tokenizer;
        }
        tokenizer = std::make_shared<Tokenizer>(tokenizer_name);
        this->addElement(tokenizer);
        return tokenizer;
    }

    Analysis::Analysis() : Object("analysis") {}
    Analyzer::ptr Analysis::analyzer()
    {
        Analyzer::ptr analyzer;
        analyzer = this->getElement<Analyzer>("analyzer");
        if (analyzer)
        {
            return analyzer;
        }
        analyzer = std::make_shared<Analyzer>();
        this->addElement(analyzer);
        return analyzer;
    }

    Settings::Settings() : Object("settings") {}
    Analysis::ptr Settings::analysis()
    {
        Analysis::ptr analysis;
        analysis = this->getElement<Analysis>("analysis");
        if (analysis)
        {
            return analysis;
        }
        analysis = std::make_shared<Analysis>();
        this->addElement(analysis);
        return analysis;
    }

    Field::Field(const std::string &key) : Object(key)
    {
        this->type("text"); // 默认使用 text 类型，调用方可覆盖
    }
    void Field::type(const std::string &type)
    {
        this->add("type", type);
    }
    void Field::boost(double boost)
    {
        this->add("boost", boost);
    }
    void Field::index(bool flag)
    {
        this->add("index", flag);
    }
    void Field::analyzer(const std::string &analizer_name)
    {
        this->add("analyzer", analizer_name);
    }

    Properties::Properties() : Object("properties") {} // 构造properties
    Field::ptr Properties::field(const std::string &field_name)
    {
        Field::ptr field;
        field = this->getElement<Field>(field_name);
        if (field)
        {
            return field;
        }
        field = std::make_shared<Field>(field_name);
        this->addElement(field);
        return field;
    }

    Mappings::Mappings() : Object("mappings")
    {
        // 默认关闭动态映射，字段通常需要通过 properties 显式定义。
        this->dynamic(false);
    }
    void Mappings::dynamic(bool flag)
    {
        this->add("dynamic", flag);
    }
    Properties::ptr Mappings::properties()
    {
        Properties::ptr properties;
        properties = this->getElement<Properties>("properties");
        if (properties)
        {
            return properties;
        }
        properties = std::make_shared<Properties>();
        this->addElement(properties);
        return properties;
    }

    // 组装查询条件及布尔查询结构。
    Term::Term(const std::string &field) : Object("term"), _field(field) {}
    Terms::Terms(const std::string &field) : Object("terms"), _field(field) {}
    Match::Match(const std::string &field) : Object("match"), _field(field) {}
    MultiMatch::MultiMatch() : Object("multi_match") {}
    void MultiMatch::appendField(const std::string &field) { this->append("fields", field); }
    Range::Range(const std::string &field) : Object("range")
    {
        // 范围边界放在字段名对应的子对象中。
        _sub = std::make_shared<Object>(field);
        this->addElement(_sub);
    }

    QObject::QObject(const std::string &key) : Object(key) {}
    Term::ptr QObject::term(const std::string &field)
    {
        Term::ptr term;
        term = this->getElement<Term>("term");
        if (term)
        {
            // 一个 term 查询只能指定一个字段；多个字段应放入 bool.must 数组。
            if (term->field() != field)
            {
                throw std::invalid_argument("term 查询只能使用一个字段，请使用 bool.must 组合多个条件");
            }
            return term;
        }
        term = std::make_shared<Term>(field);
        this->addElement(term);
        return term;
    }
    Terms::ptr QObject::terms(const std::string &field)
    {
        Terms::ptr terms;
        terms = this->getElement<Terms>("terms");
        if (terms)
        {
            if (terms->field() != field)
            {
                throw std::invalid_argument("terms 查询只能使用一个字段，请使用 bool.must 组合多个条件");
            }
            return terms;
        }
        terms = std::make_shared<Terms>(field);
        this->addElement(terms);
        return terms;
    }
    Match::ptr QObject::match(const std::string &field)
    {
        Match::ptr match;
        match = this->getElement<Match>("match");
        if (match)
        {
            if (match->field() != field)
            {
                throw std::invalid_argument("match 查询只能使用一个字段，请使用 bool.must 组合多个条件");
            }
            return match;
        }
        match = std::make_shared<Match>(field);
        this->addElement(match);
        return match;
    }
    MultiMatch::ptr QObject::multi_match()
    {
        MultiMatch::ptr multi_match;
        multi_match = this->getElement<MultiMatch>("multi_match");
        if (multi_match)
        {
            return multi_match;
        }
        multi_match = std::make_shared<MultiMatch>();
        this->addElement(multi_match);
        return multi_match;
    }
    Range::ptr QObject::range(const std::string &field)
    {
        Range::ptr range;
        range = this->getElement<Range>("range");
        if (range)
        {
            if (range->field() != field)
            {
                throw std::invalid_argument("range 查询只能使用一个字段，请使用 bool.must 组合多个条件");
            }
            return range;
        }
        range = std::make_shared<Range>(field);
        this->addElement(range);
        return range;
    }

    QArray::QArray(const std::string &key) : Array(key) {}
    Term::ptr QArray::term(const std::string &field)
    {
        // 数组中的每个条件都需要一个独立对象，例如 {"term":{"name":"张三"}}。
        auto tmp = std::make_shared<Term>(field);
        auto obj = this->newObject("");
        obj->addElement(tmp);
        return tmp;
    }
    Terms::ptr QArray::terms(const std::string &field)
    {
        auto tmp = std::make_shared<Terms>(field);
        auto obj = this->newObject("");
        obj->addElement(tmp);
        return tmp;
    }
    Match::ptr QArray::match(const std::string &field)
    {
        auto tmp = std::make_shared<Match>(field);
        auto obj = this->newObject("");
        obj->addElement(tmp);
        return tmp;
    }
    MultiMatch::ptr QArray::multi_match()
    {
        auto tmp = std::make_shared<MultiMatch>();
        auto obj = this->newObject("");
        obj->addElement(tmp);
        return tmp;
    }
    Range::ptr QArray::range(const std::string &field)
    {
        auto tmp = std::make_shared<Range>(field);
        auto obj = this->newObject("");
        obj->addElement(tmp);
        return tmp;
    }

    Must::Must() : QArray("must") {}
    Should::Should() : QArray("should") {}
    MustNot::MustNot() : QArray("must_not") {}

    QBool::QBool() : QObject("bool") {}
    void QBool::minimum_should_match(size_t count)
    {
        this->add("minimum_should_match", count);
    }
    Must::ptr QBool::must()
    {
        Must::ptr tmp;
        tmp = this->getElement<Must>("must");
        if (tmp)
        {
            return tmp;
        }
        tmp = std::make_shared<Must>();
        this->addElement(tmp);
        return tmp;
    }
    Should::ptr QBool::should()
    {
        Should::ptr tmp;
        tmp = this->getElement<Should>("should");
        if (tmp)
        {
            return tmp;
        }
        tmp = std::make_shared<Should>();
        this->addElement(tmp);
        return tmp;
    }
    MustNot::ptr QBool::must_not()
    {
        MustNot::ptr tmp;
        tmp = this->getElement<MustNot>("must_not");
        if (tmp)
        {
            return tmp;
        }
        tmp = std::make_shared<MustNot>();
        this->addElement(tmp);
        return tmp;
    }

    Query::Query() : QObject("query") {}
    void Query::match_all()
    {
        // match_all 的值是空对象，而不是字符串或 null。
        this->add("match_all", Json::Value(Json::ValueType::objectValue));
    }
    QBool::ptr Query::qbool()
    {
        QBool::ptr tmp;
        tmp = this->getElement<QBool>("bool");
        if (tmp)
        {
            return tmp;
        }
        tmp = std::make_shared<QBool>();
        this->addElement(tmp);
        return tmp;
    }
    Must::ptr Query::must()
    {
        return this->qbool()->must();
    }
    Should::ptr Query::should()
    {
        return this->qbool()->should();
    }
    MustNot::ptr Query::must_not()
    {
        return this->qbool()->must_not();
    }

    // 保存请求路径所需的信息，并构造各类操作对象。
    Request::Request(const std::string &index, const std::string &type, const std::string &op, const std::string &id)
        : _index(index), _type(type), _op(op), _id(id) {}
    void Request::set_index(const std::string &index) { _index = index; }
    void Request::set_type(const std::string &type) { _type = type; }
    void Request::set_op(const std::string &op) { _op = op; }
    void Request::set_id(const std::string &id) { _id = id; }
    const std::string &Request::index() const { return _index; }
    const std::string &Request::type() const { return _type; }
    const std::string &Request::op() const { return _op; }
    const std::string &Request::id() const { return _id; }

    Indexer::Indexer(const std::string &index) : Object(""), Request(index, "_doc", "index", index) {}
    Settings::ptr Indexer::settings()
    {
        Settings::ptr settings;
        settings = this->getElement<Settings>("settings");
        if (settings)
        {
            return settings;
        }
        settings = std::make_shared<Settings>();
        this->addElement(settings);
        return settings;
    }
    Tokenizer::ptr Indexer::tokenizer(const std::string &tokenizer_name)
    {
        return this->settings()->analysis()->analyzer()->tokenizer(tokenizer_name);
    }
    Mappings::ptr Indexer::mappings()
    {
        // 用与 JSON 节点一致的 mappings 名称查找，避免重复创建时覆盖已有字段。
        Mappings::ptr mappings;
        mappings = this->getElement<Mappings>("mappings");
        if (mappings)
        {
            return mappings;
        }
        mappings = std::make_shared<Mappings>();
        this->addElement(mappings);
        return mappings;
    }
    Field::ptr Indexer::field(const std::string &field_name)
    {
        return this->mappings()->properties()->field(field_name);
    }

    Inserter::Inserter(const std::string &index, const std::string &id) : Object(""), Request(index, "_doc", "_insert", id) {}
    Updater::Updater(const std::string &index, const std::string &id) : Object(""), Request(index, "_doc", "_update", id) {}
    Object::ptr Updater::doc()
    {
        // ES 局部更新的字段需要放在请求正文的 doc 对象中。
        Object::ptr doc;
        doc = this->getElement<Object>("doc");
        if (doc)
        {
            return doc;
        }
        doc = std::make_shared<Object>("doc");
        this->addElement(doc);
        return doc;
    }
    Deleter::Deleter(const std::string &index, const std::string &id) : Object(""), Request(index, "_doc", "_delete", id) {}
    Searcher::Searcher(const std::string &index) : Object(""), Request(index, "_doc", "_search", "") {}
    Query::ptr Searcher::query()
    {
        Query::ptr query;
        query = this->getElement<Query>("query");
        if (query)
        {
            return query;
        }
        query = std::make_shared<Query>();
        this->addElement(query);
        return query;
    }
    void Searcher::size(size_t count)
    {
        this->add("size", count);
    }
    void Searcher::from(size_t offset)
    {
        this->add("from", offset);
    }

    // 将请求对象转为 elasticlient 调用，检查 HTTP 响应。
    ESClient::ESClient(const std::vector<std::string> &hosts)
        : _client(std::make_shared<elasticlient::Client>(hosts)) {} // 支持构造一个集群客户端，发送响应只需要一个节点响应即可
    bool ESClient::create(const Indexer &idx)
    {
        // 保留参考实现：当前调用的是文档索引接口，不能保证创建索引映射。
        // 创建索引接口的修正尚未获批准，调用方使用前需要注意这一限制。
        // 1. 获取关键信息
        std::string index_name = idx.index();
        std::string index_type = idx.type();
        std::string index_id = idx.id();
        std::string index_body = idx.to_string();
        // 2. 发起请求
        auto resp = _client->index(index_name, index_type, index_id, index_body);
        // 3. 收到响应，判断是否成功
        if (resp.status_code < 200 || resp.status_code >= 300)
        {
            ERR("创建索引失败: {}/{}/{}/{} - {}", index_name, index_type, index_id, index_body, resp.text);
            return false;
        }
        return true;
    }
    bool ESClient::insert(const Inserter &ins)
    {
        // 1. 获取关键信息
        std::string index_name = ins.index();
        std::string index_type = ins.type();
        std::string doc_id = ins.id();
        std::string index_body = ins.to_string();
        // elasticlient::index 发送文档内容，使用调用方给定的文档 ID。
        auto resp = _client->index(index_name, index_type, doc_id, index_body);
        // 3. 收到响应，判断是否成功
        if (resp.status_code < 200 || resp.status_code >= 300)
        {
            ERR("新增数据失败: {}/{}/{}/{} - {}", index_name, index_type, doc_id, index_body, resp.text);
            return false;
        }
        return true;
    }
    bool ESClient::update(const Updater &upd)
    {
        // 1. 获取关键信息
        std::string index_name = upd.index();
        std::string index_type = upd.type();
        std::string doc_id = upd.id();
        std::string index_body = upd.to_string();
        // POST 到更新接口，请求正文的 doc 包含需要更新的字段。
        std::string url = index_name + "/_update/" + doc_id;
        auto resp = _client->performRequest(elasticlient::Client::HTTPMethod::POST, url, index_body);
        if (resp.status_code < 200 || resp.status_code >= 300)
        {
            ERR("更新数据失败: {}/{}/{} - {}", index_name, url, index_body, resp.text);
            return false;
        }
        return true;
    }
    bool ESClient::remove(const Deleter &del)
    {
        // 按索引、类型和文档 ID 删除一条文档。
        std::string index_name = del.index();
        std::string index_type = del.type();
        std::string doc_id = del.id();
        auto resp = _client->remove(index_name, index_type, doc_id);
        if (resp.status_code < 200 || resp.status_code >= 300)
        {
            ERR("删除数据失败: {}/{}/{} - {}", index_name, index_type, doc_id, resp.text);
            return false;
        }
        return true;
    }
    bool ESClient::remove(const std::string &index_name)
    {
        // 这个重载删除整个索引，请确认索引名后再调用。
        auto resp = _client->performRequest(elasticlient::Client::HTTPMethod::DELETE, index_name, "");
        if (resp.status_code < 200 || resp.status_code >= 300)
        {
            ERR("删除索引失败: {} - {}", index_name, resp.text);
            return false;
        }
        return true;
    }
    std::optional<Json::Value> ESClient::search(const Searcher &sea)
    {
        // 1. 获取关键信息
        std::string index_name = sea.index();
        std::string index_type = sea.type();
        std::string index_body = sea.to_string();
        auto resp = _client->search(index_name, index_type, index_body);
        if (resp.status_code < 200 || resp.status_code >= 300)
        {
            ERR("搜索数据失败: {}/{}/{} - {}", index_name, index_type, index_body, resp.text);
            return std::optional<Json::Value>();
        }
        // 请求成功后解析响应，取得 hits.hits 中每条命中的 _source。
        auto json_resp = BreezeUtil::JSON::unserialize(resp.text);
        if (!json_resp)
        {
            ERR("搜索数据,对响应进行反序列化失败: {}/{}/{} - {}", index_name, index_type, index_body, resp.text);
            return std::optional<Json::Value>();
        }
        if ((*json_resp).isNull() || (*json_resp)["hits"].isNull() || (*json_resp)["hits"]["hits"].isNull())
        {
            ERR("搜索数据,响应格式错误: {}/{}/{} - {}", index_name, index_type, index_body, resp.text);
            return std::optional<Json::Value>();
        }
        // 零条命中也返回空数组，保证成功结果的类型一致。
        Json::Value result(Json::arrayValue);
        int sz = (*json_resp)["hits"]["hits"].size();
        for (int i = 0; i < sz; ++i)
        {
            result.append((*json_resp)["hits"]["hits"][i]["_source"]);
        }
        return result;
    }

    BaseClient::ptr ESClientFactory::create(const std::vector<std::string> &hosts)
    {
        return std::make_shared<ESClient>(hosts); //
    }

}
