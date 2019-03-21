/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */

#pragma once

#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/http_client_plugin/http_client_plugin.hpp>

#include <appbase/application.hpp>

namespace eosio {

using boost::signals2::signal;

class producer_plugin : public appbase::plugin<producer_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES((chain_plugin)(http_client_plugin))

   struct runtime_options {
      fc::optional<int32_t> max_transaction_time;
      fc::optional<int32_t> max_irreversible_block_age;
      fc::optional<int32_t> produce_time_offset_us;
      fc::optional<int32_t> last_block_time_offset_us;
      fc::optional<int32_t> max_scheduled_transaction_time_per_block_ms;
      fc::optional<int32_t> subjective_cpu_leeway_us;
      fc::optional<double>  incoming_defer_ratio;
   };

   struct whitelist_blacklist {
      fc::optional< flat_set<account_name> > actor_whitelist;
      fc::optional< flat_set<account_name> > actor_blacklist;
      fc::optional< flat_set<account_name> > contract_whitelist;
      fc::optional< flat_set<account_name> > contract_blacklist;
      fc::optional< flat_set< std::pair<account_name, action_name> > > action_blacklist;
      fc::optional< flat_set<public_key_type> > key_blacklist;
   };

   struct greylist_params {
      std::vector<account_name> accounts;
   };

   struct integrity_hash_information {
      chain::block_id_type head_block_id;
      chain::digest_type   integrity_hash;
   };

   struct snapshot_information {
      chain::block_id_type head_block_id;
      std::string          snapshot_name;
   };

   producer_plugin();
   virtual ~producer_plugin();

   virtual void set_program_options(
      boost::program_options::options_description &command_line_options,
      boost::program_options::options_description &config_file_options
      ) override;


   // 判断一个公钥是否是当前区块生产者的公钥
   bool                   is_producer_key(const chain::public_key_type& key) const; 
   // 应该是对摘要进行签名，这里面的key应该是私钥。        
   chain::signature_type  sign_compact(const chain::public_key_type& key, const fc::sha256& digest) const;

   // 对插件进行初始化
   virtual void plugin_initialize(const boost::program_options::variables_map& options);
   // 插件启动函数
   virtual void plugin_startup();
   // 插件关闭函数
   virtual void plugin_shutdown();
   // 暂停运行和继续运行
   void pause();
   void resume();
   // 返回插件是否被暂停的状态
   bool paused() const;

   // 更新运行的相关参数
   void update_runtime_options(const runtime_options& options);

   // 获取运行时的相关参数
   runtime_options get_runtime_options() const;

   // 增加grey_list
   void add_greylist_accounts(const greylist_params& params);
   // 删除grey_list
   void remove_greylist_accounts(const greylist_params& params);
   // 获取greylist
   greylist_params get_greylist() const;

   // 获取whitelist和设置whitelist
   whitelist_blacklist get_whitelist_blacklist() const;
   void set_whitelist_blacklist(const whitelist_blacklist& params);

   // 获取integrity_hash_information
   integrity_hash_information get_integrity_hash() const;
   
   // 创建一个快照，snapshot中文意思是快照
   snapshot_information create_snapshot() const;

   // 定义一个返回值为空，传入变量为const chain::producer_confirmation&的signal
   // 这个信号用于区块的确认。
   signal<void(const chain::producer_confirmation&)> confirmed_block;
private:
    // producer_plugin_impl的指针,负责所有任务的执行
   std::shared_ptr<class producer_plugin_impl> my; 
};

} //eosio

FC_REFLECT(eosio::producer_plugin::runtime_options, (max_transaction_time)(max_irreversible_block_age)(produce_time_offset_us)(last_block_time_offset_us)(subjective_cpu_leeway_us)(incoming_defer_ratio));
FC_REFLECT(eosio::producer_plugin::greylist_params, (accounts));
FC_REFLECT(eosio::producer_plugin::whitelist_blacklist, (actor_whitelist)(actor_blacklist)(contract_whitelist)(contract_blacklist)(action_blacklist)(key_blacklist) )
FC_REFLECT(eosio::producer_plugin::integrity_hash_information, (head_block_id)(integrity_hash))
FC_REFLECT(eosio::producer_plugin::snapshot_information, (head_block_id)(snapshot_name))
