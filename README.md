#### 这个库主要用于网游服务器的数据库操作
>1. 提供简单的select，update，delete，insert，自定义的query，call
>2. select，update，delete，insert支持缓存功能，读写都有缓存，游戏服务器中写的比例相对较高，这里的写缓存用writeback方式实现。
>3. 自定义的query，call查询条件的参数做了防SQL注入，当然简单的SQL也是做了防SQL注入
>4. 采用连接池的方式处理SQL操作，为了防止出现SQL操作的时序问题，一般会根据主键来确定哪个连接。
>5. 类似select, query,update_r这种需要响应的操作有超时机制，并且在service端有简单的过载保护机制。
>6. 整个库是基于protobuf的反射机制来实现，将proto中的字段跟数据库中表的字段相对应。
>7. 数据库表中以json格式存储的字段支持异构数据。

#### 例子1（对于一个id一行的表，对应游戏里面玩家基本属性）:
	CREATE TABLE `player_base` (
	  `id` int(10) unsigned NOT NULL,
	  `name` blob NOT NULL,
	  PRIMARY KEY (`id`)
	) ENGINE=InnoDB DEFAULT CHARSET=latin1;

	然后需要定义一个跟表对应的proto文件
	message player_base {
		option (primary_key) = "id";
		uint32 id = 1;
		bytes name = 2;
	}

	message player_base_set {
		repeated player_base data_set = 1;
	}

	这里player_base_set不是必须的，如果需要一条SQL来查询多条数据就需要player_base_set（采用query的方式）

	接下来代码中就可以操作了
	proto::db::player_base msg1;
	msg1.set_id(100);
	msg1.set_name("aa");
	dbClient.insert(&msg1);

	proto::db::player_base msg2;
	msg2.set_id(100);
	msg2.set_name("张三");
	dbClient.update(&msg2);

	dbClient.select(100, "player_base", 10, 0, [](uint32_t nErrCode, const google::protobuf::Message* pMessage, uint64_t nContext)
	{
		const proto::db::player_base* pBase = dynamic_cast<const proto::db::player_base*>(pMessage);
		if (nullptr == pBase)
			return;

		cout << "id: " << pBase->id() << " name: " << pBase->name() << endl;
	});
	
	std::vector<db::CDbVariant> vecArgs;
	vecArgs.push_back(100);
	dbClient.query(0, "player_base", "id={0}", vecArgs, 10, 0, [](uint32_t nErrCode, const google::protobuf::Message* pMessage, uint64_t nContext)
	{
		const proto::db::player_base_set* pBaseSet = dynamic_cast<const proto::db::player_base_set*>(pMessage); //这里因为query有可能会返回多行，所以这里必须用player_base_set
		if (nullptr == pBaseSet)
			return;

		for (int32_t i = 0; i < pBaseSet->data_set_size(); ++i)
		{
			const proto::db::player_base& base = pBaseSet->data_set(i);
			cout << "id: " << base.id() << " name: " << base.name() << endl;
		}
	});

#### 例子2（对于一个id拥有多个项的，对应游戏里面玩家道具，任务）:
	CREATE TABLE `player_extend` (
        `id` int(10) unsigned NOT NULL,
        `data_set` text NOT NULL,
        PRIMARY KEY (`id`)
    ) ENGINE=InnoDB DEFAULT CHARSET=latin1;
    
    message player_extend_data {
	    optional uint32 data1 = 1;
	    optional uint32 data2 = 2;
    }

    message player_extend_data_set {
        repeated player_extend_data data = 1;
    }

    message player_extend {
	    option (primary_key) = "id";
	    optional uint32 id = 1;
	    optional player_extend_data_set data_set = 2[(serialize_type) = 1];
    }

    message player_extend_set {
	    repeated player_extend data_set = 1;
    }

    proto::db::player_extend msg1;
	msg1.set_id(100);
	proto::db::player_extend_data* pData = msg1.mutable_data_set()->add_data();
	pData->set_data1(555);
	pData->set_data2(666);
	pData = msg1.mutable_data_set()->add_data();
	pData->set_data1(888);
	pData->set_data2(999);

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

	dbClient.select(100, "player_extend", 10, 0, [](uint32_t nErrCode, const google::protobuf::Message* pMessage, uint64_t nContext)
	{
		const proto::db::player_extend* pBase = dynamic_cast<const proto::db::player_extend*>(pMessage);
		if (nullptr == pBase)
			return;

		const google::protobuf::Message& subMessage = pBase->data_set();
		std::string szText;
		google::protobuf::util::MessageToJsonString(subMessage, &szText).ok();

		cout << "id: " << pBase->id() << " name: " << szText << endl;
	});

	std::vector<db::CDbVariant> vecArgs;
	vecArgs.push_back(100);
	dbClient.query(0, "player_extend", "id={0}", vecArgs, 10, 0, [](uint32_t nErrCode, const google::protobuf::Message* pMessage, uint64_t nContext)
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
	
#### 例子3（类似在某种情况下是某种结构，某种情况下又是另一种结构）:
    CREATE TABLE `player_extend1` (
      `id` int(10) unsigned NOT NULL,
      `data_set` text NOT NULL,
      PRIMARY KEY (`id`)
    ) ENGINE=InnoDB DEFAULT CHARSET=latin1;
    
    message player_extend1_data {
    	optional uint32 data1 = 1;
    	optional uint32 data2 = 2;
    }

    message player_extend1_data_set {
    	repeated player_extend1_data data = 1;
    }
    
    message player_extend1 {
    	option (primary_key) = "id";
    	optional uint32 id = 1;
    	optional google.protobuf.Any data_set = 2[(serialize_type) = 1];
    }
    
    message player_extend1_set {
    	repeated player_extend1 data_set = 1;
    }
    
    proto::db::player_extend1 msg1;
	msg1.set_id(100);
	proto::db::player_extend1_data_set ds;
	proto::db::player_extend1_data* pData = ds.add_data();
	pData->set_data1(555);
	pData->set_data2(666);
	pData = ds.add_data();
	pData->set_data1(888);
	pData->set_data2(999);
	msg1.mutable_data_set()->PackFrom(ds);

	dbClient.update(&msg1);

	dbClient.select(100, "player_extend1", 10, 0, [](uint32_t nErrCode, const google::protobuf::Message* pMessage, uint64_t nContext)
	{
		const proto::db::player_extend1* pBase = dynamic_cast<const proto::db::player_extend1*>(pMessage);
		if (nullptr == pBase)
			return;

		const google::protobuf::Message& subMessage = pBase->data_set();
		std::string szText;
		google::protobuf::util::MessageToJsonString(subMessage, &szText).ok();

		cout << "id: " << pBase->id() << " name: " << szText << endl;
	});

	std::vector<db::CDbVariant> vecArgs;
	vecArgs.push_back(100);
	dbClient.query(0, "player_extend1", "id={0}", vecArgs, 10, 0, [](uint32_t nErrCode, const google::protobuf::Message* pMessage, uint64_t nContext)
	{
		const proto::db::player_extend1_set* pBaseSet = dynamic_cast<const proto::db::player_extend1_set*>(pMessage);
		if (nullptr == pBaseSet)
			return;

		for (int32_t i = 0; i < pBaseSet->data_set_size(); ++i)
		{
			const proto::db::player_extend1& base = pBaseSet->data_set(i);
			const google::protobuf::Message& subMessage = base.data_set();
			std::string szText;
			google::protobuf::util::MessageToJsonString(subMessage, &szText).ok();

			cout << "id: " << base.id() << " name: " << szText << endl;
		}
	})