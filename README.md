# db_service
###这个库主要用于网游服务器的数据库操作
	提供简单的select，update，delete，insert，自定义的query，call
	其中select，update，delete，insert支持缓存功能，读写都有缓存，游戏游戏服务器中写的比例相对较高，这里的写缓存用writeback方式实现。
	自定义的query，call查询条件的参数做了防SQL注入，当然简单的SQL也是做了防SQL注入
	采用连接池的方式处理SQL操作，为了防止出现SQL操作的时序问题，一般会根据主键来确定哪个连接。

###例子:

	SQL
	CREATE TABLE `player_base` (
	  `id` int(10) unsigned NOT NULL,
	  `name` blob NOT NULL,
	  PRIMARY KEY (`id`)
	) ENGINE=InnoDB DEFAULT CHARSET=latin1;

	然后需要定义一个跟表对应的proto文件
	message player_base {
		option (primary_key) = "id";
		sfixed32 id = 1;
		bytes name = 2;
	}

	message player_base_set {
		repeated player_base data_set = 1;
	}

	这里player_base_set不是必须的，如果需要一条SQL来查询多条数据就需要player_base_set

	接下来代码中就可以操作了
	proto::db::player_base msg1;
	msg1.set_id(100);
	msg1.set_name("aa");
	dbClient.insert(&msg1);

	proto::db::player_base msg2;
	msg2.set_id(100);
	msg2.set_name("张三");
	dbClient.update(&msg2);

	dbClient.select(100, "player_base", 0, [](uint32_t nErrCode, const google::protobuf::Message* pMessage, uint64_t nContext)
	{
		const proto::db::player_base* pBase = dynamic_cast<const proto::db::player_base*>(pMessage);
		if (nullptr == pBase)
			return;

		cout << "id: " << pBase->id() << " name: " << pBase->name() << endl;
	});

	如果一个id拥有多个项的（类似游戏里面道具，任务）请看player_extend

###整个库是基于protobuf的反射机制来实现
