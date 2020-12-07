/**
 *  @file
 *  @copyright defined in eosio/LICENSE.txt
 */
#include <appbase/application.hpp>

#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/net_plugin/net_plugin.hpp>
#include <eosio/producer_plugin/producer_plugin.hpp>

#include <fc/log/logger_config.hpp>
#include <fc/log/appender.hpp>
#include <fc/exception/exception.hpp>

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include "config.hpp"

using namespace appbase;
using namespace eosio;

namespace fc {
   std::unordered_map<std::string,appender::ptr>& get_appender_map();
}

namespace detail {

void configure_logging(const bfs::path& config_path)
{
   try {
      try {
         fc::configure_logging(config_path);
      } catch (...) {
         elog("Error reloading logging.json");
         throw;
      }
   } catch (const fc::exception& e) {
      elog("${e}", ("e",e.to_detail_string()));
   } catch (const boost::exception& e) {
      elog("${e}", ("e",boost::diagnostic_information(e)));
   } catch (const std::exception& e) {
      elog("${e}", ("e",e.what()));
   } catch (...) {
      // empty
   }
}

} // namespace detail

void logging_conf_loop()
{
   std::shared_ptr<boost::asio::signal_set> sighup_set(new boost::asio::signal_set(app().get_io_service(), SIGHUP));
   sighup_set->async_wait([sighup_set](const boost::system::error_code& err, int /*num*/) {
      if(!err)
      {
         ilog("Received HUP.  Reloading logging configuration.");
         auto config_path = app().get_logging_conf();
         if(fc::exists(config_path))
            ::detail::configure_logging(config_path);
         for(auto iter : fc::get_appender_map())
            iter.second->initialize(app().get_io_service());
         logging_conf_loop();
      }
   });
}

void initialize_logging()
{
   auto config_path = app().get_logging_conf();
   if(fc::exists(config_path))
     fc::configure_logging(config_path); // intentionally allowing exceptions to escape
   for(auto iter : fc::get_appender_map())
     iter.second->initialize(app().get_io_service());

   logging_conf_loop();
}

enum return_codes {
   OTHER_FAIL        = -2,
   INITIALIZE_FAIL   = -1,
   SUCCESS           = 0,
   BAD_ALLOC         = 1,
   DATABASE_DIRTY    = 2,
   FIXED_REVERSIBLE  = 3,
   EXTRACTED_GENESIS = 4,
   NODE_MANAGEMENT_SUCCESS = 5
};

// 这个相当于eos的服务端,通过添加插件的方式为客户端提供API

int main(int argc, char** argv)
{
   try {
      app().set_version(eosio::nodeos::config::version);// 设置版本号

      // 设置数据文件保存路径
      auto root = fc::app_path();
      app().set_default_data_dir(root / "eosio/nodeos/data" );
      // 设置配置文件保存路径
      app().set_default_config_dir(root / "eosio/nodeos/config" );
      http_plugin::set_defaults({
         .address_config_prefix = "",
         .default_unix_socket_path = "",
         .default_http_port = 8888
      });
      // 初始化4种插件,命令行启动后,根据用户输入的命令启动插件,区块链,http,网络以及生产区块的插件
      // 如果初始化失败,则程序停止运行
      // http_plugin与cleos交互
      // producer_plugin生产区块
      // 调用初始化函数之后, initialized_plugins中存储 已经初始化的插件
      if(!app().initialize<chain_plugin, http_plugin, net_plugin, producer_plugin>(argc, argv))    // application.hpp 77行
         return INITIALIZE_FAIL;
      
      // 初始化日志
      initialize_logging();
      // 记录日志
      ilog("nodeos version ${ver}", ("ver", app().version_string()));
      ilog("eosio root is ${root}", ("root", root.string()));
      ilog("nodeos using configuration file ${c}", ("c", app().full_config_file_path().string()));
      ilog("nodeos data directory is ${d}", ("d", app().data_dir().string()));
      
      
      app().startup();//搞这么久,这个才是重点,启动其他相关插件,开始工作

      // 运行
      app().exec();
   } catch( const extract_genesis_state_exception& e ) {
      return EXTRACTED_GENESIS;
   } catch( const fixed_reversible_db_exception& e ) {
      return FIXED_REVERSIBLE;
   } catch( const node_management_success& e ) {
      return NODE_MANAGEMENT_SUCCESS;
   } catch( const fc::exception& e ) {
      if( e.code() == fc::std_exception_code ) {
         if( e.top_message().find( "database dirty flag set" ) != std::string::npos ) {
            elog( "database dirty flag set (likely due to unclean shutdown): replay required" );
            return DATABASE_DIRTY;
         } else if( e.top_message().find( "database metadata dirty flag set" ) != std::string::npos ) {
            elog( "database metadata dirty flag set (likely due to unclean shutdown): replay required" );
            return DATABASE_DIRTY;
         }
      }
      elog( "${e}", ("e", e.to_detail_string()));
      return OTHER_FAIL;
   } catch( const boost::interprocess::bad_alloc& e ) {
      elog("bad alloc");
      return BAD_ALLOC;
   } catch( const boost::exception& e ) {
      elog("${e}", ("e",boost::diagnostic_information(e)));
      return OTHER_FAIL;
   } catch( const std::runtime_error& e ) {
      if( std::string(e.what()) == "database dirty flag set" ) {
         elog( "database dirty flag set (likely due to unclean shutdown): replay required" );
         return DATABASE_DIRTY;
      } else if( std::string(e.what()) == "database metadata dirty flag set" ) {
         elog( "database metadata dirty flag set (likely due to unclean shutdown): replay required" );
         return DATABASE_DIRTY;
      } else {
         elog( "${e}", ("e",e.what()));
      }
      return OTHER_FAIL;
   } catch( const std::exception& e ) {
      elog("${e}", ("e",e.what()));
      return OTHER_FAIL;
   } catch( ... ) {
      elog("unknown exception");
      return OTHER_FAIL;
   }

   return SUCCESS;
}
