#include "../comm/etcd.hpp"
#include <gflags/gflags.h>

DEFINE_bool(run_mode, false, "程序的运行模式，false-调试  true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志的输出等级");
DEFINE_string(etcd_host, "http://127.0.0.1:2339", "服务注册中心地址");
DEFINE_string(base_service, "/service", "服务监控根目录");

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    zchat::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);
    return 0;
}