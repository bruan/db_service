/*
MySQL Data Transfer
Source Host: localhost
Source Database: test
Target Host: localhost
Target Database: test
Date: 2017/1/24 17:59:12
*/

SET FOREIGN_KEY_CHECKS=0;
-- ----------------------------
-- Table structure for player_base
-- ----------------------------
CREATE TABLE `player_base` (
  `id` int(10) unsigned NOT NULL,
  `name` blob NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for player_extend
-- ----------------------------
CREATE TABLE `player_extend` (
  `id` int(10) unsigned NOT NULL,
  `data_set` text NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Table structure for player_extend1
-- ----------------------------
CREATE TABLE `player_extend1` (
  `id` int(10) unsigned NOT NULL,
  `data_set` text NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- ----------------------------
-- Records 
-- ----------------------------
