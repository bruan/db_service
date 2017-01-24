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

#define _DB_NAMESPACE "proto.db."

namespace db {

	enum ESerializeType
	{
		eST_Unknown			= 0,
		eST_Json			= 1,
		eST_Protobuf_Bin	= 2,
	};

	class CLoadProtobufErrorCollector : 
		public google::protobuf::compiler::MultiFileErrorCollector
	{
	public:
		CLoadProtobufErrorCollector() {}
		~CLoadProtobufErrorCollector() {}

		void AddError(const std::string& filename, int line, int column, const std::string& message)
		{
			PrintWarning("protobuf error: %s,%d,%d,%s", filename.c_str(), line, column, message.c_str());
		}
	};

	class CProtobufTextParserErrorCollector : 
		public google::protobuf::io::ErrorCollector
	{
	public:
		void AddError(int line, int column, const std::string& message)
		{
			PrintWarning("ERROR: Parse text. line = %d, column = %d, error = %s", line, column, message.c_str());
		}

		void AddWarning(int line, int column, const std::string& message)
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

		bool init(const std::string& szDir, const std::vector<std::string>& vecFile);

		google::protobuf::Message* createMessage(const std::string& szName);

	private:
		google::protobuf::compiler::Importer*		m_pImporter;
		google::protobuf::DynamicMessageFactory*	m_pFactory;
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

	bool CMessageFactory::init(const std::string& szDir, const std::vector<std::string>& vecFile)
	{
		this->m_pFactory = new google::protobuf::DynamicMessageFactory();

		google::protobuf::compiler::DiskSourceTree sourceTree;
		CLoadProtobufErrorCollector errorColloctor;
		this->m_pImporter = new google::protobuf::compiler::Importer(&sourceTree, &errorColloctor);

		sourceTree.MapPath("", szDir);

		DebugAstEx(this->m_pImporter->Import("google/protobuf/any.proto"), false);
		for (auto iter = vecFile.begin(); iter != vecFile.end(); ++iter)
		{
			DebugAstEx(this->m_pImporter->Import(*iter), false);
		}

		return true;
	}

	google::protobuf::Message* CMessageFactory::createMessage(const std::string& szName)
	{
		google::protobuf::Message* pMessage = nullptr;
		const google::protobuf::Descriptor* pDescriptor = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(szName);
		if (pDescriptor != nullptr)
		{
			const google::protobuf::Message* pProto = google::protobuf::MessageFactory::generated_factory()->GetPrototype(pDescriptor);
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

	bool importProtobuf(const std::string& szDir, const std::vector<std::string>& vecFile)
	{
		return CMessageFactory::Inst()->init(szDir, vecFile);
	}

	std::string getMessageNameByTableName(const std::string& szTableName)
	{
		return std::string(_DB_NAMESPACE) + szTableName;
	}

	bool getTableNameByMessageName(const std::string& szMessageName, std::string& szTableName)
	{
		static size_t nPrefixLen = strlen(_DB_NAMESPACE);

		size_t pos = szMessageName.find(_DB_NAMESPACE);
		if (pos == std::string::npos)
			return false;

		szTableName = szMessageName.substr(pos + nPrefixLen);
		return true;
	}

	static bool getBasicTypeFieldValue(const google::protobuf::Message* pMessage, const google::protobuf::FieldDescriptor* pFieldDescriptor, const google::protobuf::Reflection* pReflection, std::string& szValue)
	{
		DebugAstEx(pMessage != nullptr, false);
		DebugAstEx(pFieldDescriptor != nullptr, false);
		DebugAstEx(pReflection != nullptr, false);

		switch (pFieldDescriptor->type())
		{
		case google::protobuf::FieldDescriptor::TYPE_INT32:
		case google::protobuf::FieldDescriptor::TYPE_SINT32:
		case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
			{
				int32_t nValue = pReflection->GetInt32(*pMessage, pFieldDescriptor);
				std::ostringstream oss;
				oss << nValue;
				szValue = oss.str();
			}
			break;

		case google::protobuf::FieldDescriptor::TYPE_UINT32:
		case google::protobuf::FieldDescriptor::TYPE_FIXED32:
			{
				uint32_t nValue = pReflection->GetUInt32(*pMessage, pFieldDescriptor);
				std::ostringstream oss;
				oss << nValue;
				szValue = oss.str();
			}
			break;

		case google::protobuf::FieldDescriptor::TYPE_INT64:
		case google::protobuf::FieldDescriptor::TYPE_SINT64:
		case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
			{
				int64_t nValue = pReflection->GetInt64(*pMessage, pFieldDescriptor);
				std::ostringstream oss;
				oss << nValue;
				szValue = oss.str();
			}
			break;

		case google::protobuf::FieldDescriptor::TYPE_UINT64:
		case google::protobuf::FieldDescriptor::TYPE_FIXED64:
			{
				uint64_t nValue = pReflection->GetUInt64(*pMessage, pFieldDescriptor);
				std::ostringstream oss;
				oss << nValue;
				szValue = oss.str();
			}
			break;

		case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
			{
				double nValue = pReflection->GetDouble(*pMessage, pFieldDescriptor);
				std::ostringstream oss;
				oss << nValue;
				szValue = oss.str();
			}
			break;

		case google::protobuf::FieldDescriptor::TYPE_FLOAT:
			{
				float nValue = pReflection->GetFloat(*pMessage, pFieldDescriptor);
				std::ostringstream oss;
				oss << nValue;
				szValue = oss.str();
			}
			break;

		case google::protobuf::FieldDescriptor::TYPE_STRING:
		case google::protobuf::FieldDescriptor::TYPE_BYTES:
			szValue = pReflection->GetString(*pMessage, pFieldDescriptor);
			break;

		default:
			DebugAstEx(false, false);
		}

		return true;
	}

	bool setBasicTypeFieldValue(google::protobuf::Message* pMessage, const google::protobuf::Reflection* pReflection, const google::protobuf::FieldDescriptor* pFieldDescriptor, const std::string& szValue)
	{
		DebugAstEx(pMessage != nullptr, false);
		DebugAstEx(pReflection != nullptr, false);
		DebugAstEx(pFieldDescriptor != nullptr, false);

		switch (pFieldDescriptor->type())
		{
		case google::protobuf::FieldDescriptor::TYPE_INT32:
		case google::protobuf::FieldDescriptor::TYPE_SINT32:
		case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
			{
				int32_t nValue = 0;
				std::istringstream iss(szValue);
				iss >> nValue;
				pReflection->SetInt32(pMessage, pFieldDescriptor, nValue);
			}
			break;

		case google::protobuf::FieldDescriptor::TYPE_UINT32:
		case google::protobuf::FieldDescriptor::TYPE_FIXED32:
			{
				uint32_t nValue = 0;
				std::istringstream iss(szValue);
				iss >> nValue;
				pReflection->SetUInt32(pMessage, pFieldDescriptor, nValue);
			}
			break;

		case google::protobuf::FieldDescriptor::TYPE_INT64:
		case google::protobuf::FieldDescriptor::TYPE_SINT64:
		case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
			{
				int64_t nValue = 0;
				std::istringstream iss(szValue);
				iss >> nValue;
				pReflection->SetInt64(pMessage, pFieldDescriptor, nValue);
			}
			break;

		case google::protobuf::FieldDescriptor::TYPE_UINT64:
		case google::protobuf::FieldDescriptor::TYPE_FIXED64:
			{
				uint64_t nValue = 0;
				std::istringstream iss(szValue);
				iss >> nValue;
				pReflection->SetUInt64(pMessage, pFieldDescriptor, nValue);
			}
			break;

		case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
			{
				double nValue = 0.0;
				std::istringstream oss(szValue);
				oss >> nValue;
				pReflection->SetDouble(pMessage, pFieldDescriptor, nValue);
			}
			break;

		case google::protobuf::FieldDescriptor::TYPE_FLOAT:
			{
				float nValue = 0.0f;
				std::istringstream oss(szValue);
				oss >> nValue;
				pReflection->SetDouble(pMessage, pFieldDescriptor, nValue);
			}
			break;

		case google::protobuf::FieldDescriptor::TYPE_STRING:
		case google::protobuf::FieldDescriptor::TYPE_BYTES:
			{
				pReflection->SetString(pMessage, pFieldDescriptor, szValue);
			}
			break;

		default:
			DebugAstEx(false, false);
		}

		return true;
	}

	bool getMessageFieldInfos(const google::protobuf::Message* pMessage, std::vector<SFieldInfo>& vecFieldInfo)
	{
		DebugAstEx(pMessage != nullptr, false);

		const google::protobuf::Descriptor* pDescriptor = pMessage->GetDescriptor();
		DebugAstEx(pDescriptor != nullptr, false);

		const google::protobuf::Reflection* pReflection = pMessage->GetReflection();
		DebugAstEx(pReflection != nullptr, false);

		for (int32_t i = 0; i < pDescriptor->field_count(); ++i)
		{
			const google::protobuf::FieldDescriptor* pFieldDescriptor = pDescriptor->field(i);
			DebugAstEx(pFieldDescriptor != nullptr, false);
			DebugAstEx(pFieldDescriptor->label() == google::protobuf::FieldDescriptor::LABEL_OPTIONAL, false);
			
			if (pFieldDescriptor->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE)
			{
				ESerializeType eSerializeType = (ESerializeType)pFieldDescriptor->options().GetExtension(serialize_type);
				
#ifdef GetMessage
#undef GetMessage
#endif
				std::string szBuf;
				switch (eSerializeType)
				{
				case eST_Protobuf_Bin:
					{
						const google::protobuf::Message& subMessage = pReflection->GetMessage(*pMessage, pFieldDescriptor);
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
						const google::protobuf::Message& subMessage = pReflection->GetMessage(*pMessage, pFieldDescriptor);

						if (!google::protobuf::util::MessageToJsonString(subMessage, &szBuf).ok())
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
				sFieldInfo.szValue = std::move(szBuf);
				sFieldInfo.bStr = true;

				vecFieldInfo.push_back(sFieldInfo);
			}
			else
			{
				std::string szValue;
				if (!getBasicTypeFieldValue(pMessage, pFieldDescriptor, pReflection, szValue))
				{
					PrintWarning("ERROR: message[%s] can't get field[%s] value", pMessage->GetTypeName().c_str(), pFieldDescriptor->name().c_str());
					return false;
				}

				SFieldInfo sFieldInfo;
				sFieldInfo.szName = pFieldDescriptor->name();
				sFieldInfo.szValue = std::move(szValue);
				sFieldInfo.bStr = (pFieldDescriptor->type() == google::protobuf::FieldDescriptor::TYPE_STRING || pFieldDescriptor->type() == google::protobuf::FieldDescriptor::TYPE_BYTES);
				
				vecFieldInfo.push_back(sFieldInfo);
			}
		}

		return true;
	}

	static bool setFieldValue(google::protobuf::Message* pMessage, const google::protobuf::Reflection* pReflection, const google::protobuf::FieldDescriptor* pFieldDescriptor, const std::string& szValue)
	{
		DebugAstEx(pMessage != nullptr, false);
		DebugAstEx(pReflection != nullptr, false);
		DebugAstEx(pFieldDescriptor != nullptr, false);
		DebugAstEx(pFieldDescriptor->label() == google::protobuf::FieldDescriptor::LABEL_OPTIONAL, false);

		if (pFieldDescriptor->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE)
		{
			ESerializeType eSerializeType = (ESerializeType)pFieldDescriptor->options().GetExtension(serialize_type);

			switch (eSerializeType)
			{
			case eST_Protobuf_Bin:
				{
					google::protobuf::Message* pSubMessage = pReflection->MutableMessage(pMessage, pFieldDescriptor);
					DebugAstEx(pSubMessage != nullptr, false);
					DebugAstEx(pSubMessage->ParseFromString(szValue), false);
				}
				break;

			case eST_Json:
				{
					google::protobuf::Message* pSubMessage = pReflection->MutableMessage(pMessage, pFieldDescriptor);
					DebugAstEx(pSubMessage != nullptr, false);
					DebugAstEx(google::protobuf::util::JsonStringToMessage(szValue, pSubMessage).ok(), false);
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

	std::string getPrimaryName(const google::protobuf::Message* pMessage)
	{
		DebugAstEx(pMessage != nullptr, "");

		const google::protobuf::Reflection* pReflection = pMessage->GetReflection();
		DebugAstEx(pReflection != nullptr, "");

		const google::protobuf::Descriptor* pDescriptor = pMessage->GetDescriptor();
		DebugAstEx(pDescriptor != nullptr, "");

		return pDescriptor->options().GetExtension(primary_key);
	}

	google::protobuf::Message* createRepeatMessage(CDbRecordset* pDbRecordset, const std::string& szName)
	{
		DebugAstEx(pDbRecordset != nullptr, nullptr);

		std::string szMessageName(szName + "_set");
		google::protobuf::Message* pMessage = CMessageFactory::Inst()->createMessage(szMessageName);
		DebugAstEx(pMessage != nullptr, nullptr);

		const google::protobuf::Reflection* pMainReflection = pMessage->GetReflection();
		if (pMainReflection == nullptr)
		{
			PrintWarning("message[%s] can't get reflection.", szMessageName.c_str());
			delete pMessage;
			return nullptr;
		}

		const google::protobuf::Descriptor* pMainDescriptor = pMessage->GetDescriptor();
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

		const google::protobuf::FieldDescriptor* pMainFieldDescriptor = pMainDescriptor->field(0);
		if (pMainFieldDescriptor == nullptr)
		{
			PrintWarning("message[%s] can't get field descriptor.", szMessageName.c_str());
			delete pMessage;
			return nullptr;
		}

		if (pMainFieldDescriptor->label() != google::protobuf::FieldDescriptor::LABEL_REPEATED ||
			pMainFieldDescriptor->type() != google::protobuf::FieldDescriptor::TYPE_MESSAGE)
		{
			PrintWarning("message[%s] main field prototy is wrong.", szMessageName.c_str());
			delete pMessage;
			return nullptr;
		}

		for (uint64_t row = 0; row < pDbRecordset->getRowCount(); ++row)
		{
			pDbRecordset->fatchNextRow();
			google::protobuf::Message* pSubMessage = pMainReflection->AddMessage(pMessage, pMainFieldDescriptor);
			if (pSubMessage == nullptr || pSubMessage->GetTypeName() != szName)
			{
				PrintWarning("message[%s] AddMessage failed.", szMessageName.c_str());
				delete pMessage;
				return nullptr;
			}

			const google::protobuf::Reflection* pReflection = pSubMessage->GetReflection();
			if (pReflection == nullptr)
			{
				PrintWarning("message[%s] can't get reflection.", pSubMessage->GetTypeName().c_str());
				delete pMessage;
				return nullptr;
			}

			const google::protobuf::Descriptor* pDescriptor = pSubMessage->GetDescriptor();
			if (pDescriptor == nullptr)
			{
				PrintWarning("message[%s] can't get descriptor.", pSubMessage->GetTypeName().c_str());
				delete pMessage;
				return nullptr;
			}

			for (uint32_t i = 0; i < pDbRecordset->getFieldCount(); ++i)
			{
				const std::string& szFieldName = pDbRecordset->getFieldName(i);
				const std::string& szValue = pDbRecordset->getData(i);

				const google::protobuf::FieldDescriptor* pFieldDescriptor = pDescriptor->FindFieldByName(szFieldName);
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

	bool fillNormalMessage(CDbRecordset* pDbRecordset, google::protobuf::Message* pMessage)
	{
		DebugAstEx(pDbRecordset != nullptr, false);
		DebugAstEx(pDbRecordset->getRowCount() <= 1, false);

		if (pDbRecordset->getRowCount() == 0)
			return true;

		const google::protobuf::Reflection* pReflection = pMessage->GetReflection();
		if (pReflection == nullptr)
		{
			PrintWarning("message[%s] can't get reflection.", pMessage->GetTypeName.c_str());
			return false;
		}

		const google::protobuf::Descriptor* pDescriptor = pMessage->GetDescriptor();
		if (pDescriptor == NULL)
		{
			PrintWarning("message[%s] can't get descriptor.", pMessage->GetTypeName.c_str());
			return false;
		}

		pDbRecordset->fatchNextRow();
		
		for (uint32_t i = 0; i < pDbRecordset->getFieldCount(); ++i)
		{
			const std::string& szFieldName = pDbRecordset->getFieldName(i);
			const std::string& szValue = pDbRecordset->getData(i);
			
			const google::protobuf::FieldDescriptor* pFieldDescriptor = pDescriptor->FindFieldByName(szFieldName);
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

	google::protobuf::Message* createMessage(const std::string& szName)
	{
		return CMessageFactory::Inst()->createMessage(szName);
	}
}