/**
 * Created by leituo56 on 11/23/14.
 */
var sqlite3 = require('sqlite3').verbose();
var db_cfg = new sqlite3.Database('/mnt/glusterfs/cfg.db');
var db_lease = new sqlite3.Database('/mnt/glusterfs/lease.db');

exports.db_cfg = db_cfg;
exports.db_lease = db_lease;
