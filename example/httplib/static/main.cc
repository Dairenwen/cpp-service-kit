#include <httplib.h>

// 将工作目录中的 webroot 映射到网站根路径，提供静态页面服务。
int main()
{
    httplib::Server svr;
    // 根路径请求自动使用 webroot/index.html。
    svr.set_mount_point("/", "./webroot");
    svr.listen("0.0.0.0", 8080);
    return 0;
}
