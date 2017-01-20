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
		kMET_LostConnection			= 1,	// 连接丢失
		kMET_NoPrivileges			= 1044, // 数据库用户权限不足
		kMET_NoDiskSpace			= 1021, // 硬盘剩余空间不足
		kMET_KeyDupForUpdate		= 1022, // 关键字重复，更改记录失败
		kMET_NoRecord				= 1032, // 记录不存在
		kMET_NoMemory				= 1037, // 系统内存不足
		kMET_OutofConnection		= 1040, // 已到达数据库的最大连接数，请加大数据库可用连接数
		kMET_SQLSyntax				= 1149, // SQL语句语法错误
		kMET_KeyDupForInsert		= 1062,	// 关键字重复，插入记录失败
		kMET_CommitTrans			= 1180, // 提交事务失败
		kMET_RollbackTrans			= 1181, // 回滚事务失败
		kMET_Deadloop				= 1205, // 加锁超时
		kMET_StatementReprepared	= 1615,	// Statement需要重新准备

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