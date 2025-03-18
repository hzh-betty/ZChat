#include "../../comm/logger.hpp"
#include <gflags/gflags.h>

DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    zchat::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);
    LOG_DEBUG("你好：{}", "betty");
    LOG_INFO("你好：{}", "betty");
    LOG_WARN("你好：{}", "betty");
    LOG_ERROR("你好：{}", "betty");
    LOG_FATAL("你好：{}", "betty");
    LOG_DEBUG("这是一个测试");
    return 0;
}