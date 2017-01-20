#pragma once

#include <string>

#include "db_service_base.h"

#include <windows.h>
#include "mysql/mysql.h"

namespace db
{
	enum EMysqlErrorType
	{
		kMET_OK						= 0,
		kMET_LostConnection			= 1,	// ���Ӷ�ʧ
		kMET_NoPrivileges			= 1044, // ���ݿ��û�Ȩ�޲���
		kMET_NoDiskSpace			= 1021, // Ӳ��ʣ��ռ䲻��
		kMET_KeyDupForUpdate		= 1022, // �ؼ����ظ������ļ�¼ʧ��
		kMET_NoRecord				= 1032, // ��¼������
		kMET_NoMemory				= 1037, // ϵͳ�ڴ治��
		kMET_OutofConnection		= 1040, // �ѵ������ݿ���������������Ӵ����ݿ����������
		kMET_SQLSyntax				= 1149, // SQL����﷨����
		kMET_KeyDupForInsert		= 1062,	// �ؼ����ظ��������¼ʧ��
		kMET_CommitTrans			= 1180, // �ύ����ʧ��
		kMET_RollbackTrans			= 1181, // �ع�����ʧ��
		kMET_Deadloop				= 1205, // ������ʱ
		kMET_StatementReprepared	= 1615,	// Statement��Ҫ����׼��

		kMET_Unknwon = -1
	};


	class CDbRecordset;
	class CDbConnection
	{
		friend class CDbFacade;

	public:
		CDbConnection();
		virtual ~CDbConnection();

		bool			connect(const std::string& szHost, uint16_t nPort, const std::string& szUser, const std::string& szPassword, const std::string& szDbname, const std::string& szCharacterset);
		bool			isConnect() const;
		void			close();
		
		uint32_t		execute(const std::string& szSQL, CDbRecordset** pDbRecordset);
		bool			ping();
		uint64_t		getAffectedRow() const;
		void			enableAutoCommit(bool enable);
		void			begintrans();
		void			endtrans();
		void			rollback();
		std::string		escape(const std::string& szSQL);

	private:
		MYSQL*						m_pMysql;	
	};
}