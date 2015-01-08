#include <fstream>
#include "include/base/cef_logging.h"

#include <third-party/json/json.h>
#include <sys/stat.h>
#include "account_manager.h"
#include "platform_util.h"

AccountManager::AccountManager()
   : current_account_(NULL),
     last_id_(0)
{
}

AccountManager::~AccountManager() {
}

bool
AccountManager::AddAccount(const CefRefPtr<Account> account) {
  accounts_[++last_id_] = account;
  account->SetId(last_id_);
  LOG(WARNING)<< "Acccount added: "<< last_id_;
  return true;
}

bool
AccountManager::DeleteAccount(int id) {
  if (accounts_.count(id) == 0)
    return false;

  accounts_.erase(id);
  return true;
}

bool
AccountManager::SwitchAccount(int id) {
  if (accounts_.count(id) == 0)
    return false;

  current_account_ = accounts_[id];
  return true;
}


bool
AccountManager::Commit() {
  Json::Value json(Json::objectValue);
  Json::Value json_accounts(Json::arrayValue);

  accounts_map::iterator it = accounts_.begin();
  for (; it != accounts_.end(); ++it) {
    CefRefPtr<Account> account = (*it).second;
    Json::Value json_account(Json::objectValue);
    json_account["secure"] = account->IsSecure();
    json_account["domain"] = account->GetDomain();
    json_account["login"] = account->GetLogin();
    json_account["password"] = account->GetPassword();
    json_account["default"] = account == current_account_;
    json_accounts.append(json_account);
  }
  json["accounts"] = json_accounts;
  std::ofstream ofs(config_path_);
  ofs << json;
  chmod(config_path_.c_str(), S_IRUSR|S_IWUSR);
  return true;
}

CefRefPtr<Account>
AccountManager::GetCurrentAccount() {
  return current_account_;
}

AccountManager::accounts_map*
AccountManager::GetAccounts() {
  return &accounts_;
}

void
AccountManager::Init(std::string config_path) {
  Json::Value json;   // will contains the json value after parsing.
  Json::Reader reader;
  config_path_ = config_path;
  std::ifstream ifs(config_path);
  std::string config(
     (std::istreambuf_iterator<char>(ifs) ),
     (std::istreambuf_iterator<char>()    )
  );

  bool parsingSuccessful = reader.parse(config, json);
  if (!parsingSuccessful) {
    LOG(ERROR) << "Failed to parse configuration\n"
       << reader.getFormattedErrorMessages();
    return;
  }

  const Json::Value accounts = json["accounts"];
  for(unsigned int i=0; i < accounts.size(); ++i) {
    CefRefPtr<Account> account(new Account);
    account->SetLogin(accounts[i].get("login", "").asString());
    account->SetPassword(accounts[i].get("password", "").asString());
    account->SetSecure(accounts[i].get("secure", false).asBool());
    account->SetDomain(accounts[i].get("domain", "").asString());

    AddAccount(account);
    if (accounts[i].get("default", false).asBool()) {
      SwitchAccount(account->GetId());
    }
  }
}