/**
 * Copyright Soramitsu Co., Ltd. 2017 All Rights Reserved.
 * http://soramitsu.co.jp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ametsuchi/query_serializer.hpp"
#include <queries.pb.h>
#include <algorithm>

namespace iroha {
  namespace ametsuchi {

    using namespace rapidjson;

    QuerySerializer::QuerySerializer() {
      deserializers_["get_account"] = &QuerySerializer::deserializeGetAccount;
      deserializers_["get_account_assets"] =
          &QuerySerializer::deserializeGetAccountAssets;
      deserializers_["get_account_asset_transactions"] =
          &QuerySerializer::deserializeGetAccountAssetTransactions;
      deserializers_["get_account_transactions"] =
          &QuerySerializer::deserializeGetAccountTransactions;
      deserializers_["get_account_signatories"] =
          &QuerySerializer::deserializeGetSignatories;
    }

    nonstd::optional<protocol::Query> QuerySerializer::deserialize(
        const std::string query_json) {
      protocol::Query pb_query;
      Document doc;
      if (doc.Parse(query_json.c_str()).HasParseError()) {
        return nonstd::nullopt;
      }

      // check if all necessary fields are there
      auto obj_query = doc.GetObject();
      auto req_fields = {"signature",  "creator_account_id", "created_ts",
                         "query_hash", "query_counter",      "query_type"};
      if (std::any_of(req_fields.begin(), req_fields.end(),
                      [&obj_query](auto &&field) {
                        return not obj_query.HasMember(field);
                      })) {
        return nonstd::nullopt;
      }

      auto sig = obj_query["signature"].GetObject();

      // check if signature has all needed fields
      if (not sig.HasMember("pubkey")) {
        return nonstd::nullopt;
      }
      if (not sig.HasMember("signature")) {
        return nonstd::nullopt;
      }

      auto pb_header = pb_query.mutable_header();
      auto pb_sig = pb_header->mutable_signature();
      pb_sig->set_pubkey(sig["pubkey"].GetString());
      pb_sig->set_signature(sig["signature"].GetString());

      // set creator account id
      pb_query.set_creator_account_id(
          obj_query["creator_account_id"].GetString());

      // set query counter
      pb_query.set_query_counter(obj_query["query_counter"].GetUint64());

      auto query_type = obj_query["query_type"].GetString();

      auto it = deserializers_.find(query_type);
      if (it != deserializers_.end() and (this->*it->second)(obj_query, pb_query)) {
        return pb_query;
      } else {
        return nonstd::nullopt;
      }
    }

    bool QuerySerializer::deserializeGetAccount(
        rapidjson::GenericValue<rapidjson::UTF8<char>>::Object &obj_query,
        protocol::Query &pb_query) {
      if (not obj_query.HasMember("account_id")) {
        return false;
      }
      auto pb_get_account = pb_query.mutable_get_account();
      pb_get_account->set_account_id(obj_query["account_id"].GetString());

      return true;
    }

    bool QuerySerializer::deserializeGetSignatories(
        rapidjson::GenericValue<rapidjson::UTF8<char>>::Object &obj_query,
        protocol::Query &pb_query) {
      if (not obj_query.HasMember("account_id")) {
        return false;
      }
      auto pb_get_signatories = pb_query.mutable_get_account_signatories();
      pb_get_signatories->set_account_id(obj_query["account_id"].GetString());

      return true;
    }

    bool QuerySerializer::deserializeGetAccountTransactions(
        rapidjson::GenericValue<rapidjson::UTF8<char>>::Object &obj_query,
        protocol::Query &pb_query) {
      // check if all fields exist
      if (not obj_query.HasMember("account_id")) {
        return false;
      }
      auto pb_get_account_transactions =
          pb_query.mutable_get_account_transactions();
      pb_get_account_transactions->set_account_id(
          obj_query["account_id"].GetString());

      return true;
    }

    bool QuerySerializer::deserializeGetAccountAssetTransactions(
        rapidjson::GenericValue<rapidjson::UTF8<char>>::Object &obj_query,
        protocol::Query &pb_query) {
      // check if all fields exist
      if (not(obj_query.HasMember("account_id") &&
              obj_query.HasMember("asset_id"))) {
        return false;
      }

      auto pb_get_account_asset_transactions =
          pb_query.mutable_get_account_asset_transactions();
      pb_get_account_asset_transactions->set_account_id(
          obj_query["account_id"].GetString());
      pb_get_account_asset_transactions->set_asset_id(
          obj_query["asset_id"].GetString());

      return true;
    }

    bool QuerySerializer::deserializeGetAccountAssets(
        rapidjson::GenericValue<rapidjson::UTF8<char>>::Object &obj_query,
        protocol::Query &pb_query) {
      // check if all fields exist
      if (not(obj_query.HasMember("account_id") &&
              obj_query.HasMember("asset_id"))) {
        return false;
      }
      auto pb_get_account_assets = pb_query.mutable_get_account_assets();
      pb_get_account_assets->set_account_id(
          obj_query["account_id"].GetString());
      pb_get_account_assets->set_asset_id(obj_query["asset_id"].GetString());

      return true;
    }
  }
}