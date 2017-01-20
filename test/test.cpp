// test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "../db_service/inc/db_service.h"
#include "player_base.pb.h"
#include <Windows.h>
#include "select_command.pb.h"
#include "query_command.pb.h"
#include "call_command.pb.h"

#include "../db_proxy.h"
#include "../db_client.h"

#include <iostream>
#include "../db_service/src/db_command_handler.h"
#include "result_set.pb.h"
#include "player_extend.pb.h"
#include "google/protobuf/util/json_util.h"

using namespace std;

class CMyDbProxy : public db::CDbProxy
{
public:
	CMyDbProxy()
	{
		this->m_pDbThreadMgr = db::create("127.0.0.1", 3306, "test", "root", "123456", "utf8", 5);
	}

	virtual ~CMyDbProxy()
	{
		db::release(this->m_pDbThreadMgr);
	}

	void update()
	{
		std::list<db::SDbResultInfo> listDbResultInfo;
		db::getResultInfo(this->m_pDbThreadMgr, listDbResultInfo);
		for (auto iter = listDbResultInfo.begin(); iter != listDbResultInfo.end(); ++iter)
		{
			this->onMessage(iter->pResponse.get());
		}
	}

protected:
	virtual bool sendRequest(const proto::db::request* pRequest)
	{
		db::query(this->m_pDbThreadMgr, 0, pRequest);

		return true;
	}

private:
	db::CDbThreadMgr* m_pDbThreadMgr;
};

int _tmain(int argc, _TCHAR* argv[])
{
	std::list<int> l1;
	std::list<int> l2;
	l1.push_back(1);
	l1.push_back(2);
	l2.push_back(11);
	l2.push_back(12);

	l1.splice(l1.begin(), l2, l2.begin(), l2.end());


	CMyDbProxy myDbProxy;
	db::CDbClient dbClient(&myDbProxy);
	dbClient.nop(100, 0, [](uint32_t nErrCode, const google::protobuf::Message* pMessage, uint64_t nContext)
	{
		cout << "nop" << endl;
	});
	
	{
		dbClient.remove(100, "player_base");

		proto::db::player_base msg1;
		msg1.set_id(100);
		msg1.set_name("aa");

		dbClient.insert(&msg1);

		proto::db::player_base msg2;
		msg2.set_id(200);
		msg2.set_name("bb");
		dbClient.update(&msg2);

		dbClient.select(200, "player_base", 0, [](uint32_t nErrCode, const google::protobuf::Message* pMessage, uint64_t nContext)
		{
			const proto::db::player_base* pBase = dynamic_cast<const proto::db::player_base*>(pMessage);
			if (nullptr == pBase)
				return;

			cout << "id: " << pBase->id() << " name: " << pBase->name() << endl;
		});

		std::vector<db::CVariant> vecArgs;
		vecArgs.push_back(100);
		dbClient.query(0, "player_base", "id={0}", vecArgs, 0, [](uint32_t nErrCode, const google::protobuf::Message* pMessage, uint64_t nContext)
		{
			const proto::db::player_base_set* pBaseSet = dynamic_cast<const proto::db::player_base_set*>(pMessage);
			if (nullptr == pBaseSet)
				return;

			for (int32_t i = 0; i < pBaseSet->data_set_size(); ++i)
			{
				const proto::db::player_base& base = pBaseSet->data_set(i);
				cout << "id: " << base.id() << " name: " << base.name() << endl;
			}
		});

		vecArgs.clear();
		vecArgs.push_back(200);
		dbClient.call_r(0, "select *from player_base where id = {0}", vecArgs, 0, [](uint32_t nErrCode, const google::protobuf::Message* pMessage, uint64_t nContext)
		{
			const proto::db::result_set* pResultset = dynamic_cast<const proto::db::result_set*>(pMessage);
			if (nullptr == pResultset)
				return;

			for (int32_t i = 0; i < pResultset->rows_size(); ++i)
			{
				const proto::db::row& row = pResultset->rows(i);
				for (int32_t j = 0; j < row.value_size(); ++j)
				{
					cout << "name: " << pResultset->field_name(j) << " value: " << row.value(j);
				}
				cout << endl;
			}
		});
	}
	{
		proto::db::player_extend msg1;
		msg1.set_id(100);
		proto::db::player_extend_data* pData = msg1.mutable_data_set()->add_data();
		pData->set_data1(555);
		pData->set_data2(666);
		pData = msg1.mutable_data_set()->add_data();
		pData->set_data1(888);
		pData->set_data2(999);

		const google::protobuf::FieldDescriptor* pFieldDescriptor = msg1.GetDescriptor()->field(1);

		dbClient.insert(&msg1);

		proto::db::player_extend msg2;
		msg2.set_id(100);
		pData = msg2.mutable_data_set()->add_data();
		pData->set_data1(1555);
		pData->set_data2(1666);
		pData = msg2.mutable_data_set()->add_data();
		pData->set_data1(1888);
		pData->set_data2(1999);
		dbClient.update(&msg2);

		dbClient.select(100, "player_extend", 0, [](uint32_t nErrCode, const google::protobuf::Message* pMessage, uint64_t nContext)
		{
			const proto::db::player_extend* pBase = dynamic_cast<const proto::db::player_extend*>(pMessage);
			if (nullptr == pBase)
				return;

			const google::protobuf::Message& subMessage = pBase->data_set();
			std::string szText;
			google::protobuf::util::MessageToJsonString(subMessage, &szText).ok();

			cout << "id: " << pBase->id() << " name: " << szText << endl;
		});

		std::vector<db::CVariant> vecArgs;
		vecArgs.push_back(100);
		dbClient.query(0, "player_extend", "id={0}", vecArgs, 0, [](uint32_t nErrCode, const google::protobuf::Message* pMessage, uint64_t nContext)
		{
			const proto::db::player_extend_set* pBaseSet = dynamic_cast<const proto::db::player_extend_set*>(pMessage);
			if (nullptr == pBaseSet)
				return;

			for (int32_t i = 0; i < pBaseSet->data_set_size(); ++i)
			{
				const proto::db::player_extend& base = pBaseSet->data_set(i);
				const google::protobuf::Message& subMessage = base.data_set();
				std::string szText;
				google::protobuf::util::MessageToJsonString(subMessage, &szText).ok();

				cout << "id: " << base.id() << " name: " << szText << endl;
			}
		});
	}

	while (true)
	{
		myDbProxy.update();
		Sleep(100);
	}

	Sleep(~0);
	return 0;
}

