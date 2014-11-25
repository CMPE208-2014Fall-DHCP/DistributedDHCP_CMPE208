/**
 * Created by leituo56 on 11/23/14.
 */
var sqlite3 = require('sqlite3').verbose();
var db = new sqlite3.Database('/var/db/dhcpd.lease');

exports.db = db;