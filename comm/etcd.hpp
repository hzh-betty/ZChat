#pragma once
#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include <etcd/Response.hpp>
#include <etcd/Watcher.hpp>
#include <etcd/Value.hpp>
#include <functional>
#include "logger.hpp"

namespace zchat
{
    // 服务注册客户端类
    class Registry
    {
    public:
        using ptr = std::shared_ptr<Registry>;
        Registry(const std::string &host) : client_(std::make_shared<etcd::Client>(host)),
                                            keepAlive_(client_->leasekeepalive(3).get()),
                                            leaseId_(keepAlive_->Lease()) {}
        ~Registry() { keepAlive_->Cancel(); }

        bool registry(const std::string &key, const std::string &val)
        {
            auto resp = client_->put(key, val, leaseId_).get();
            if (resp.is_ok() == false)
            {
                LOG_ERROR("注册数据失败：{}", resp.error_message());
                return false;
            }
            return true;
        }

    private:
        std::shared_ptr<etcd::Client> client_;
        std::shared_ptr<etcd::KeepAlive> keepAlive_;
        uint64_t leaseId_;
    };

    // 服务发现客户端类
    class Discovery
    {
    public:
        using ptr = std::shared_ptr<Discovery>;
        using NotifyCallback = std::function<void(std::string, std::string)>;
        Discovery(const std::string &host,
                  const std::string &basedir,
                  const NotifyCallback &put_cb,
                  const NotifyCallback &del_cb) : client_(std::make_shared<etcd::Client>(host)),
                                                  putCb_(put_cb), delCb_(del_cb)
        {
            // 1. 先进行服务发现,先获取到当前已有的数据
            auto resp = client_->ls(basedir).get();
            if (resp.is_ok() == false)
            {
                LOG_ERROR("获取服务信息数据失败：{}", resp.error_message());
            }
            int sz = resp.keys().size();
            for (int i = 0; i < sz; ++i)
            {
                if (putCb_)
                    putCb_(resp.key(i), resp.value(i).as_string());
            }

            // 2. 然后进行事件监控，监控数据发生的改变并调用回调进行处理
            watcher_ = std::make_shared<etcd::Watcher>(*client_.get(), basedir,
                                                       std::bind(&Discovery::callback, this, std::placeholders::_1), true);
        }

        ~Discovery()
        {
            watcher_->Cancel();
        }

    private:
        void callback(const etcd::Response &resp)
        {
            if (resp.is_ok() == false)
            {
                LOG_ERROR("收到一个错误的事件通知: {}", resp.error_message());
                return;
            }

            for (auto const &ev : resp.events())
            {
                // 1. 新增事件处理
                if (ev.event_type() == etcd::Event::EventType::PUT)
                {
                    if (putCb_)
                        putCb_(ev.kv().key(), ev.kv().as_string());
                    LOG_DEBUG("新增服务：{}-{}", ev.kv().key(), ev.kv().as_string());
                }

                // 2. 删除事件处理
                else if (ev.event_type() == etcd::Event::EventType::DELETE_)
                {
                    if (delCb_)
                        delCb_(ev.prev_kv().key(), ev.prev_kv().as_string());
                    LOG_DEBUG("下线服务：{}-{}", ev.prev_kv().key(), ev.prev_kv().as_string());
                }
            }
        }

    private:
        NotifyCallback putCb_;
        NotifyCallback delCb_;
        std::shared_ptr<etcd::Client> client_;
        std::shared_ptr<etcd::Watcher> watcher_;
    };
}