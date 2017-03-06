#include <stdlib.h>
#include <sstream>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/json_util.h"
#include "google/protobuf/compiler/importer.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/io/tokenizer.h"

#include "proto_src/db_option.pb.h"

#include "db_protobuf.h"
#include "db_common.h"
#include "db_service_base.h"

using namespace std;
using namespace google::protobuf;
using namespace db;

#define _DB_NAMESPACE "proto.db."

enum ESerializeType
{
	eST_Unknown			= 0,
	eST_Json			= 1,
	eST_Protobuf_Bin	= 2,
};

class CLoadProtobufErrorCollector : 
	public compiler::MultiFileErrorCollector
{
public:
	CLoadProtobufErrorCollector() {}
	~CLoadProtobufErrorCollector() {}

	void AddError(const string& filename, int line, int column, const string& message)
	{
		PrintWarning("protobuf error: %s,%d,%d,%s", filename.c_str(), line, column, message.c_str());
	}
};

class CProtobufTextParserErrorCollector : 
	public io::ErrorCollector
{
public:
	void AddError(int line, int column, const string& message)
	{
		PrintWarning("ERROR: Parse text. line = %d, column = %d, error = %s", line, column, message.c_str());
	}

	void AddWarning(int line, int column, const string& message)
	{
		PrintWarning("WARNING: Parse text. line = %d, column = %d, error = %s", line, column, message.c_str());
	}
};

class CMessageFactory
{
public:
	CMessageFactory();
	~CMessageFactory();

	static CMessageFactory* Inst();

	bool init(const string& szDir, const vector<string>& vecFile);

	Message* createMessage(const string& szName);

private:
	compiler::Importer*		m_pImporter;
	DynamicMessageFactory*	m_pFactory;
};

CMessageFactory::CMessageFactory()
	: m_pImporter(nullptr)
	, m_pFactory(nullptr)
{
}

CMessageFactory::~CMessageFactory()
{
	delete this->m_pFactory;
	delete this->m_pImporter;
}

CMessageFactory* CMessageFactory::Inst()
{
	static CMessageFactory s_Inst;

	return &s_Inst;
}

bool CMessageFactory::init(const string& szDir, const vector<string>& vecFile)
{
	this->m_pFactory = new DynamicMessageFactory();

	compiler::DiskSourceTree sourceTree;
	CLoadProtobufErrorCollector errorColloctor;
	this->m_pImporter = new compiler::Importer(&sourceTree, &errorColloctor);

	sourceTree.MapPath("", szDir);

	DebugAstEx(this->m_pImporter->Import("google/protobuf/any.proto"), false);
	for (auto iter = vecFile.begin(); iter != vecFile.end(); ++iter)
	{
		DebugAstEx(this->m_pImporter->Import(*iter), false);
	}

	return true;
}

Message* CMessageFactory::createMessage(const string& szName)
{
	Message* pMessage = nullptr;
	const Descriptor* pDescriptor = DescriptorPool::generated_pool()->FindMessageTypeByName(szName);
	if (pDescriptor != nullptr)
	{
		const Message* pProto = MessageFactory::generated_factory()->GetPrototype(pDescriptor);
		if (pProto != nullptr)
			pMessage = pProto->New();
	}
	else
	{
		pDescriptor = this->m_pImporter->pool()->FindMessageTypeByName(szName);
		if (pDescriptor != nullptr)
			pMessage = this->m_pFactory->GetPrototype(pDescriptor)->New();
	}

	return pMessage;
}

namespace db
{
	bool importProtobuf(const string& szDir, const vector<string>& vecFile)
	{
		return CMessageFactory::Inst()->init(szDir, vecFile);
	}

	string getMessageNameByTableName(const string& szTableName)
	{
		return string(_DB_NAMESPACE) + szTableName;
	}

	bool getTableNameByMessageName(const string& szMessageName, string& szTableName)
	{
		static size_t nPrefixLen = strlen(_DB_NAMESPACE);

		size_t pos = szMessageName.find(_DB_NAMESPACE);
		if (pos == string::npos)
			return false;

		szTableName = szMessageName.substr(pos + nPrefixLen);
		return true;
	}

	static bool getBasicTypeFieldValue(const Message* pMessage, const FieldDescriptor* pFieldDescriptor, const Reflection* pReflection, string& szValue)
	{
		DebugAstEx(pMessage != nullptr, false);
		DebugAstEx(pFieldDescriptor != nullptr, false);
		DebugAstEx(pReflection != nullptr, false);

		switch (pFieldDescriptor->type())
		{
		case FieldDescriptor::TYPE_INT32:
		case FieldDescriptor::TYPE_SINT32:
		case FieldDescriptor::TYPE_SFIXED32:
			{
				int32_t nValue = pReflection->GetInt32(*pMessage, pFieldDescriptor);
				ostringstream oss;
				oss << nValue;
				szValue = oss.str();
			}
			break;

		case FieldDescriptor::TYPE_UINT32:
		case FieldDescriptor::TYPE_FIXED32:
			{
				uint32_t nValue = pReflection->GetUInt32(*pMessage, pFieldDescriptor);
				ostringstream oss;
				oss << nValue;
				szValue = oss.str();
			}
			break;

		case FieldDescriptor::TYPE_INT64:
		case FieldDescriptor::TYPE_SINT64:
		case FieldDescriptor::TYPE_SFIXED64:
			{
				int64_t nValue = pReflection->GetInt64(*pMessage, pFieldDescriptor);
				ostringstream oss;
				oss << nValue;
				szValue = oss.str();
			}
			break;

		case FieldDescriptor::TYPE_UINT64:
		case FieldDescriptor::TYPE_FIXED64:
			{
				uint64_t nValue = pReflection->GetUInt64(*pMessage, pFieldDescriptor);
				ostringstream oss;
				oss << nValue;
				szValue = oss.str();
			}
			break;

		case FieldDescriptor::TYPE_DOUBLE:
			{
				double nValue = pReflection->GetDouble(*pMessage, pFieldDescriptor);
				ostringstream oss;
				oss << nValue;
				szValue = oss.str();
			}
			break;

		case FieldDescriptor::TYPE_FLOAT:
			{
				float nValue = pReflection->GetFloat(*pMessage, pFieldDescriptor);
				ostringstream oss;
				oss << nValue;
				szValue = oss.str();
			}
			break;

		case FieldDescriptor::TYPE_STRING:
		case FieldDescriptor::TYPE_BYTES:
			szValue = pReflection->GetString(*pMessage, pFieldDescriptor);
			break;

		default:
			DebugAstEx(false, false);
		}

		return true;
	}

	bool setBasicTypeFieldValue(Message* pMessage, const Reflection* pReflection, const FieldDescriptor* pFieldDescriptor, const string& szValue)
	{
		DebugAstEx(pMessage != nullptr, false);
		DebugAstEx(pReflection != nullptr, false);
		DebugAstEx(pFieldDescriptor != nullptr, false);

		switch (pFieldDescriptor->type())
		{
		case FieldDescriptor::TYPE_INT32:
		case FieldDescriptor::TYPE_SINT32:
		case FieldDescriptor::TYPE_SFIXED32:
		{
			int32_t nValue = 0;
			istringstream iss(szValue);
			iss >> nValue;
			pReflection->SetInt32(pMessage, pFieldDescriptor, nValue);
		}
		break;

		case FieldDescriptor::TYPE_UINT32:
		case FieldDescriptor::TYPE_FIXED32:
		{
			uint32_t nValue = 0;
			istringstream iss(szValue);
			iss >> nValue;
			pReflection->SetUInt32(pMessage, pFieldDescriptor, nValue);
		}
		break;

		case FieldDescriptor::TYPE_INT64:
		case FieldDescriptor::TYPE_SINT64:
		case FieldDescriptor::TYPE_SFIXED64:
		{
			int64_t nValue = 0;
			istringstream iss(szValue);
			iss >> nValue;
			pReflection->SetInt64(pMessage, pFieldDescriptor, nValue);
		}
		break;

		case FieldDescriptor::TYPE_UINT64:
		case FieldDescriptor::TYPE_FIXED64:
		{
			uint64_t nValue = 0;
			istringstream iss(szValue);
			iss >> nValue;
			pReflection->SetUInt64(pMessage, pFieldDescriptor, nValue);
		}
		break;

		case FieldDescriptor::TYPE_DOUBLE:
		{
			double nValue = 0.0;
			istringstream oss(szValue);
			oss >> nValue;
			pReflection->SetDouble(pMessage, pFieldDescriptor, nValue);
		}
		break;

		case FieldDescriptor::TYPE_FLOAT:
		{
			float nValue = 0.0f;
			istringstream oss(szValue);
			oss >> nValue;
			pReflection->SetDouble(pMessage, pFieldDescriptor, nValue);
		}
		break;

		case FieldDescriptor::TYPE_STRING:
		case FieldDescriptor::TYPE_BYTES:
		{
			pReflection->SetString(pMessage, pFieldDescriptor, szValue);
		}
		break;

		default:
			DebugAstEx(false, false);
		}

		return true;
	}

	bool getMessageFieldInfos(const Message* pMessage, vector<SFieldInfo>& vecFieldInfo)
	{
		DebugAstEx(pMessage != nullptr, false);

		const Descriptor* pDescriptor = pMessage->GetDescriptor();
		DebugAstEx(pDescriptor != nullptr, false);

		const Reflection* pReflection = pMessage->GetReflection();
		DebugAstEx(pReflection != nullptr, false);

		for (int32_t i = 0; i < pDescriptor->field_count(); ++i)
		{
			const FieldDescriptor* pFieldDescriptor = pDescriptor->field(i);
			DebugAstEx(pFieldDescriptor != nullptr, false);
			DebugAstEx(pFieldDescriptor->label() == FieldDescriptor::LABEL_OPTIONAL, false);

			if (pFieldDescriptor->type() == FieldDescriptor::TYPE_MESSAGE)
			{
				ESerializeType eSerializeType = (ESerializeType)pFieldDescriptor->options().GetExtension(serialize_type);

				if (!pReflection->HasField(*pMessage, pFieldDescriptor))
					continue;

#ifdef GetMessage
#undef GetMessage
#endif
				string szBuf;
				switch (eSerializeType)
				{
				case eST_Protobuf_Bin:
					{
						const Message& subMessage = pReflection->GetMessage(*pMessage, pFieldDescriptor);
						szBuf.resize(subMessage.ByteSize());
						if (subMessage.ByteSize() > 0 && !subMessage.SerializeToArray(&szBuf[0], (int32_t)szBuf.size()))
						{
							PrintWarning("SerializeToArray fail.[%s:%s]", pMessage->GetTypeName().c_str(), pFieldDescriptor->type_name());
							return false;
						}
					}
					break;

				case eST_Json:
					{
						const Message& subMessage = pReflection->GetMessage(*pMessage, pFieldDescriptor);

						if (!util::MessageToJsonString(subMessage, &szBuf).ok())
						{
							PrintWarning("MessageToJsonString fail.[%s:%s]", pMessage->GetTypeName().c_str(), pFieldDescriptor->type_name());
							return false;
						}
					}
					break;

				default:
					{
						PrintWarning("Message[%s] field[%s] hasn't serialize type.", pMessage->GetTypeName().c_str(), pFieldDescriptor->name().c_str());
						return false;
					}
				}

				SFieldInfo sFieldInfo;
				sFieldInfo.szName = pFieldDescriptor->name();
				sFieldInfo.szValue = move(szBuf);
				sFieldInfo.bStr = true;

				vecFieldInfo.push_back(sFieldInfo);
			}
			else
			{
				string szValue;
				if (!getBasicTypeFieldValue(pMessage, pFieldDescriptor, pReflection, szValue))
				{
					PrintWarning("ERROR: message[%s] can't get field[%s] value", pMessage->GetTypeName().c_str(), pFieldDescriptor->name().c_str());
					return false;
				}

				SFieldInfo sFieldInfo;
				sFieldInfo.szName = pFieldDescriptor->name();
				sFieldInfo.szValue = move(szValue);
				sFieldInfo.bStr = (pFieldDescriptor->type() == FieldDescriptor::TYPE_STRING || pFieldDescriptor->type() == FieldDescriptor::TYPE_BYTES);

				vecFieldInfo.push_back(sFieldInfo);
			}
		}

		return true;
	}

	static bool setFieldValue(Message* pMessage, const Reflection* pReflection, const FieldDescriptor* pFieldDescriptor, const string& szValue)
	{
		DebugAstEx(pMessage != nullptr, false);
		DebugAstEx(pReflection != nullptr, false);
		DebugAstEx(pFieldDescriptor != nullptr, false);
		DebugAstEx(pFieldDescriptor->label() == FieldDescriptor::LABEL_OPTIONAL, false);

		if (pFieldDescriptor->type() == FieldDescriptor::TYPE_MESSAGE)
		{
			ESerializeType eSerializeType = (ESerializeType)pFieldDescriptor->options().GetExtension(serialize_type);

			switch (eSerializeType)
			{
			case eST_Protobuf_Bin:
				{
					Message* pSubMessage = pReflection->MutableMessage(pMessage, pFieldDescriptor);
					DebugAstEx(pSubMessage != nullptr, false);
					DebugAstEx(pSubMessage->ParseFromString(szValue), false);
				}
				break;

			case eST_Json:
				{
					Message* pSubMessage = pReflection->MutableMessage(pMessage, pFieldDescriptor);
					DebugAstEx(pSubMessage != nullptr, false);
					DebugAstEx(util::JsonStringToMessage(szValue, pSubMessage).ok(), false);
				}
				break;

			default:
				DebugAstEx(false, false);
			}
		}
		else
		{
			DebugAstEx(setBasicTypeFieldValue(pMessage, pReflection, pFieldDescriptor, szValue), false);
		}

		return true;
	}

	string getPrimaryName(const Message* pMessage)
	{
		DebugAstEx(pMessage != nullptr, "");

		const Reflection* pReflection = pMessage->GetReflection();
		DebugAstEx(pReflection != nullptr, "");

		const Descriptor* pDescriptor = pMessage->GetDescriptor();
		DebugAstEx(pDescriptor != nullptr, "");

		return pDescriptor->options().GetExtension(primary_key);
	}

	bool getPrimaryValue(const Message* pMessage, uint64_t& nValue)
	{
		if (pMessage == nullptr)
			return false;

		const Reflection* pReflection = pMessage->GetReflection();
		if (pReflection == nullptr)
			return false;

		const Descriptor* pDescriptor = pMessage->GetDescriptor();
		if (pDescriptor == nullptr)
			return false;

		string szPrimaryFieldName = pDescriptor->options().GetExtension(primary_key);
		if (szPrimaryFieldName.empty())
			return false;

		const FieldDescriptor* pFieldDescriptor = pMessage->GetDescriptor()->FindFieldByName(szPrimaryFieldName);
		if (pFieldDescriptor == nullptr)
			return false;

		switch (pFieldDescriptor->type())
		{
		case FieldDescriptor::TYPE_INT32:
		case FieldDescriptor::TYPE_SINT32:
		case FieldDescriptor::TYPE_SFIXED32:
			nValue = pReflection->GetInt32(*pMessage, pFieldDescriptor);
			break;

		case FieldDescriptor::TYPE_UINT32:
		case FieldDescriptor::TYPE_FIXED32:
			nValue = pReflection->GetUInt32(*pMessage, pFieldDescriptor);
			break;

		case FieldDescriptor::TYPE_INT64:
		case FieldDescriptor::TYPE_SINT64:
		case FieldDescriptor::TYPE_SFIXED64:
			nValue = pReflection->GetInt64(*pMessage, pFieldDescriptor);
			break;

		case FieldDescriptor::TYPE_UINT64:
		case FieldDescriptor::TYPE_FIXED64:
			nValue = pReflection->GetUInt64(*pMessage, pFieldDescriptor);
			break;

		default:
			return false;
		}

		return true;
	}

	Message* createRepeatMessage(CDbRecordset* pDbRecordset, const string& szName)
	{
		DebugAstEx(pDbRecordset != nullptr, nullptr);

		string szMessageName(szName + "_set");
		Message* pMessage = CMessageFactory::Inst()->createMessage(szMessageName);
		DebugAstEx(pMessage != nullptr, nullptr);

		const Reflection* pMainReflection = pMessage->GetReflection();
		if (pMainReflection == nullptr)
		{
			PrintWarning("message[%s] can't get reflection.", szMessageName.c_str());
			delete pMessage;
			return nullptr;
		}

		const Descriptor* pMainDescriptor = pMessage->GetDescriptor();
		if (pMainDescriptor == NULL)
		{
			PrintWarning("message[%s] can't get descriptor.", szMessageName.c_str());
			delete pMessage;
			return nullptr;
		}

		if (pMainDescriptor->field_count() != 1)
		{
			PrintWarning("message[%s] field count isn't one.", szMessageName.c_str());
			delete pMessage;
			return nullptr;
		}

		const FieldDescriptor* pMainFieldDescriptor = pMainDescriptor->field(0);
		if (pMainFieldDescriptor == nullptr)
		{
			PrintWarning("message[%s] can't get field descriptor.", szMessageName.c_str());
			delete pMessage;
			return nullptr;
		}

		if (pMainFieldDescriptor->label() != FieldDescriptor::LABEL_REPEATED ||
			pMainFieldDescriptor->type() != FieldDescriptor::TYPE_MESSAGE)
		{
			PrintWarning("message[%s] main field prototy is wrong.", szMessageName.c_str());
			delete pMessage;
			return nullptr;
		}

		for (uint64_t row = 0; row < pDbRecordset->getRowCount(); ++row)
		{
			pDbRecordset->fatchNextRow();
			Message* pSubMessage = pMainReflection->AddMessage(pMessage, pMainFieldDescriptor);
			if (pSubMessage == nullptr || pSubMessage->GetTypeName() != szName)
			{
				PrintWarning("message[%s] AddMessage failed.", szMessageName.c_str());
				delete pMessage;
				return nullptr;
			}

			const Reflection* pReflection = pSubMessage->GetReflection();
			if (pReflection == nullptr)
			{
				PrintWarning("message[%s] can't get reflection.", pSubMessage->GetTypeName().c_str());
				delete pMessage;
				return nullptr;
			}

			const Descriptor* pDescriptor = pSubMessage->GetDescriptor();
			if (pDescriptor == nullptr)
			{
				PrintWarning("message[%s] can't get descriptor.", pSubMessage->GetTypeName().c_str());
				delete pMessage;
				return nullptr;
			}

			for (uint32_t i = 0; i < pDbRecordset->getFieldCount(); ++i)
			{
				const string& szFieldName = pDbRecordset->getFieldName(i);
				const string& szValue = pDbRecordset->getData(i);

				const FieldDescriptor* pFieldDescriptor = pDescriptor->FindFieldByName(szFieldName);
				if (pFieldDescriptor == nullptr)
				{
					PrintWarning("field[%s.%s] descriptor is NULL.", pSubMessage->GetTypeName().c_str(), szFieldName.c_str());
					delete pMessage;
					return nullptr;
				}

				if (!setFieldValue(pSubMessage, pReflection, pFieldDescriptor, szValue))
				{
					PrintWarning("setFieldValue[%s.%s] failed.", pSubMessage->GetTypeName().c_str(), szFieldName.c_str());
					delete pMessage;
					return nullptr;
				}
			}
		}

		return pMessage;
	}

	bool fillNormalMessage(CDbRecordset* pDbRecordset, Message* pMessage)
	{
		DebugAstEx(pDbRecordset != nullptr, false);
		DebugAstEx(pDbRecordset->getRowCount() <= 1, false);

		if (pDbRecordset->getRowCount() == 0)
			return true;

		const Reflection* pReflection = pMessage->GetReflection();
		if (pReflection == nullptr)
		{
			PrintWarning("message[%s] can't get reflection.", pMessage->GetTypeName.c_str());
			return false;
		}

		const Descriptor* pDescriptor = pMessage->GetDescriptor();
		if (pDescriptor == NULL)
		{
			PrintWarning("message[%s] can't get descriptor.", pMessage->GetTypeName.c_str());
			return false;
		}

		pDbRecordset->fatchNextRow();

		for (uint32_t i = 0; i < pDbRecordset->getFieldCount(); ++i)
		{
			const string& szFieldName = pDbRecordset->getFieldName(i);
			const string& szValue = pDbRecordset->getData(i);

			const FieldDescriptor* pFieldDescriptor = pDescriptor->FindFieldByName(szFieldName);
			if (pFieldDescriptor == nullptr)
			{
				PrintWarning("field[%s.%s] descriptor is NULL.", pMessage->GetTypeName().c_str(), szFieldName.c_str());
				delete pMessage;
				return nullptr;
			}

			if (!setFieldValue(pMessage, pReflection, pFieldDescriptor, szValue))
			{
				PrintWarning("setFieldValue[%s.%s] failed.", pMessage->GetTypeName().c_str(), szFieldName.c_str());
				delete pMessage;
				return nullptr;
			}
		}

		return true;
	}

	Message* createMessage(const string& szName)
	{
		return CMessageFactory::Inst()->createMessage(szName);
	}
}