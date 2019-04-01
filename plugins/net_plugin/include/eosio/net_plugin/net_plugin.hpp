/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/net_plugin/protocol.hpp>

namespace eosio {
   using namespace appbase;

   struct connection_status {
      string            peer;                // 表示连接的对象是谁？
      bool              connecting = false;
      bool              syncing    = false;
      handshake_message last_handshake;
   };

   class net_plugin : public appbase::plugin<net_plugin>
   {
      public:
        net_plugin();
        virtual ~net_plugin();

        APPBASE_PLUGIN_REQUIRES((chain_plugin))    // net_plugin这个插件的启动，依赖chain_plugin插件
        virtual void set_program_options(options_description& cli, options_description& cfg) override;

        void plugin_initialize(const variables_map& options);  // 插件初始化
        void plugin_startup();                                 // 插件的启动
        void plugin_shutdown();                                // 插件关闭

        void   broadcast_block(const chain::signed_block &sb); // 广播区块

        string                       connect( const string& endpoint ); // 连接其他端点
        string                       disconnect( const string& endpoint ); // 断开连接
        optional<connection_status>  status( const string& endpoint )const;   // 查看与某个端点的链接状态
        vector<connection_status>    connections()const;                   // 查看连接状态

        size_t num_peers() const;                                          // 查看建立连接的端点个数
      private:
        std::unique_ptr<class net_plugin_impl> my;   // 和producer_plugin_imple一样，这个插件负责网络中的具体操作
   };

}

FC_REFLECT( eosio::connection_status, (peer)(connecting)(syncing)(last_handshake) )
