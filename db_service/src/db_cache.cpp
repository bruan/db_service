#include "db_cache.h"
#include "db_protobuf.h"

namespace db
{
	CDbCache::CDbCache()
		: m_nDataSize(0)
	{

	}

	CDbCache::~CDbCache()
	{

	}

	std::shared_ptr<google::protobuf::Message> CDbCache::getData(uint32_t nDataID)
	{
		auto iter = this->m_mapCache.find(nDataID);
		if (iter == this->m_mapCache.end())
			return nullptr;

		return iter->second;
	}

	void CDbCache::setData(uint32_t nDataID, std::shared_ptr<google::protobuf::Message>& pData)
	{
		DebugAst(pData != nullptr);

		auto iter = this->m_mapCache.find(nDataID);
		if (iter == this->m_mapCache.end())
		{
			this->m_mapCache.insert(std::make_pair(nDataID, pData));
			this->m_nDataSize += pData->ByteSize();
			return;
		}

		std::shared_ptr<google::protobuf::Message> pDstData = iter->second;
		int32_t nSize = pDstData->ByteSize();

		const google::protobuf::Descriptor* pSrcDescriptor = pData->GetDescriptor();
		DebugAst(pSrcDescriptor != nullptr);

		const google::protobuf::Reflection* pSrcReflection = pData->GetReflection();
		DebugAst(pSrcReflection != nullptr);

		const google::protobuf::Descriptor* pDstDescriptor = pDstData->GetDescriptor();
		DebugAst(pDstDescriptor != nullptr);

		const google::protobuf::Reflection* pDstReflection = pDstData->GetReflection();
		DebugAst(pDstReflection != nullptr);

		for (int32_t i = 0; i < pSrcDescriptor->field_count(); ++i)
		{
			const google::protobuf::FieldDescriptor* pSrcFieldDescriptor = pSrcDescriptor->field(i);
			DebugAst(pSrcFieldDescriptor != nullptr);
			const google::protobuf::FieldDescriptor* pDstFieldDescriptor = pDstDescriptor->FindFieldByName(pSrcFieldDescriptor->name());
			DebugAst(pDstFieldDescriptor != nullptr);
			
			switch (pSrcFieldDescriptor->type())
			{
			case google::protobuf::FieldDescriptor::TYPE_INT32:
			case google::protobuf::FieldDescriptor::TYPE_SINT32:
			case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
				pDstReflection->SetInt32(pDstData.get(), pDstFieldDescriptor, pSrcReflection->GetInt32(*pData.get(), pSrcFieldDescriptor));
				break;

			case google::protobuf::FieldDescriptor::TYPE_UINT32:
			case google::protobuf::FieldDescriptor::TYPE_FIXED32:
				pDstReflection->SetUInt32(pDstData.get(), pDstFieldDescriptor, pSrcReflection->GetUInt32(*pData.get(), pSrcFieldDescriptor));
				break;

			case google::protobuf::FieldDescriptor::TYPE_INT64:
			case google::protobuf::FieldDescriptor::TYPE_SINT64:
			case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
				pDstReflection->SetInt64(pDstData.get(), pDstFieldDescriptor, pSrcReflection->GetInt64(*pData.get(), pSrcFieldDescriptor));
				break;

			case google::protobuf::FieldDescriptor::TYPE_UINT64:
			case google::protobuf::FieldDescriptor::TYPE_FIXED64:
				pDstReflection->SetUInt64(pDstData.get(), pDstFieldDescriptor, pSrcReflection->GetUInt64(*pData.get(), pSrcFieldDescriptor));
				break;

			case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
				pDstReflection->SetDouble(pDstData.get(), pDstFieldDescriptor, pSrcReflection->GetDouble(*pData.get(), pSrcFieldDescriptor));
				break;

			case google::protobuf::FieldDescriptor::TYPE_FLOAT:
				pDstReflection->SetFloat(pDstData.get(), pDstFieldDescriptor, pSrcReflection->GetFloat(*pData.get(), pSrcFieldDescriptor));
				break;

			case google::protobuf::FieldDescriptor::TYPE_STRING:
			case google::protobuf::FieldDescriptor::TYPE_BYTES:
				pDstReflection->SetString(pDstData.get(), pDstFieldDescriptor, pSrcReflection->GetString(*pData.get(), pSrcFieldDescriptor));
				break;

			case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
				{
#ifdef GetMessage
#undef GetMessage
#endif
					google::protobuf::Message* pDstSubMessage = pDstReflection->MutableMessage(pDstData.get(), pDstFieldDescriptor);
					const google::protobuf::Message& srcSubMessage = pSrcReflection->GetMessage(*pData.get(), pSrcFieldDescriptor);
					pDstSubMessage->CopyFrom(srcSubMessage);
				}
				break;

			default:
				DebugAst(false);
			}
		}

		this->m_nDataSize -= nSize;
		this->m_nDataSize += pDstData->ByteSize();
	}

	void CDbCache::delData(uint32_t nDataID)
	{
		auto iter = this->m_mapCache.find(nDataID);
		if (iter == this->m_mapCache.end())
			return;

		this->m_nDataSize -= iter->second->ByteSize();
		this->m_mapCache.erase(nDataID);
	}

	int32_t CDbCache::getDataSize() const
	{
		return this->m_nDataSize;
	}

}