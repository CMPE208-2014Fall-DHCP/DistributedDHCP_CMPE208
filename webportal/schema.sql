CREATE TABLE IF NOT EXISTS DHCP_CONF (key VARCHAR(60), value VARCHAR(60));

CREATE TABLE IF NOT EXISTS lease_tbl (hw_addr VARCHAR(60), state INTEGER, timeout INTEGER, creator VARCHAR(60), owner VARCHAR(60));

CREATE TABLE IF NOT EXISTS fixedip_tbl (identifer VARCHAR(60), fixed_addr VARCHAR(20));

CREATE TABLE IF NOT EXISTS active_tbl (owner VARCHAR(60), state INTEGER, heartbeat INTEGER);

--Test Data
BEGIN TRANSACTION;

INSERT INTO `lease_tbl` VALUES('84:38:35:51:e2:7e',1,500,'Tuo Lei','dhcpMac');
INSERT INTO `lease_tbl` VALUES('32:00:1e:14:20:00',2,1000,'Wei Zhong','dhcpWin');
INSERT INTO `lease_tbl` VALUES('b6:c5:68:5b:09:0c',5,51830498,'NoBody','dhcpMac');
INSERT INTO `lease_tbl` VALUES('06:38:35:51:e2:7e',0,300,'Juan','dhcpWin');

INSERT INTO `fixedip_tbl` VALUES('32:00:1e:14:20:00','172.0.0.3');
INSERT INTO `fixedip_tbl` VALUES('84:38:35:51:e2:7e','172.0.0.4');
INSERT INTO `fixedip_tbl` VALUES('06:38:35:51:e2:7e','172.0.0.5');
INSERT INTO `fixedip_tbl` VALUES('b6:c5:68:5b:09:0c','172.0.0.6');

INSERT INTO `active_tbl` VALUES('dhcpMac','alive',130);
INSERT INTO `active_tbl` VALUES('dhcpWin','dead',0);

INSERT INTO `DHCP_CONF` VALUES('name','DHCP cluster');
INSERT INTO `DHCP_CONF` VALUES('group',4);
INSERT INTO `DHCP_CONF` VALUES('OtherKey','OtherValue');
COMMIT;
--End Test Data