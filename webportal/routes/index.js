/**
 * Created by leituo56 on 11/22/14.
 */
var express = require('express');
var router = express.Router();
var db = require('./conn').db;
var IP_Utils = require('../public/javascripts/ip_util');

/* GET home page. */
router.get('/', function (req, res) {
    var KVs = [];
    var servers = [];
    var sql = "SELECT * FROM tbl_cfg";
    db.each(sql, function (err, row) {
        var keyStr = row.key;
        var valStr = row.val;

        //If key is in this array, turn int to ip address.
        var ipArr = ['range_start', 'range_end', 'subnetmask', 'router'];
        if(ipArr.indexOf(keyStr.toLowerCase())>-1)
            valStr = IP_Utils.intToIP(valStr);

        KVs.push({"key": keyStr, "val": valStr});
    }, function(){
        //GET nodes
        var sql2 = "SELECT * FROM tbl_nodes";
        db.each(sql2, function(err, row){
            servers.push({
                "node_name": row.node_name,
                "ip_num": row.server_ip,
                "server_ip": IP_Utils.intToIP(row.server_ip),
                "state": row.state,
                "heartbeat": row.heartbeat
            });
        }, function(){
            res.render('index', {
                title: 'DHCP Home',
                kvs: KVs,
                servers: servers
            });
        });
    });
});

//DHCP_CONF
router.post('/mod', function(req, res){
    var paramArr = ['Range_start', 'Range_end', 'SubnetMask', 'Router', 'LeaseTime'];
    db.serialize(function() {
        var deletion = db.prepare("DELETE FROM tbl_cfg WHERE key=?");
        var stmt = db.prepare("INSERT INTO tbl_cfg VALUES (? , ?)");
        for(var item in paramArr){
            deletion.run(paramArr[item]);
            stmt.run(paramArr[item], req.param(paramArr[item]));
        }
        deletion.finalize();
        stmt.finalize();
    });
    res.redirect('/');
});
router.post('/conf', function(req, res){
    if(req.param('key')!=null || req.param('key')==''){
        db.serialize(function() {
            var deletion = db.prepare("DELETE FROM tbl_cfg WHERE key=?");
            var stmt = db.prepare("INSERT INTO tbl_cfg VALUES (? , ?)");
            deletion.run(req.param('key'));
            stmt.run([req.param('key'), req.param('val')]);

            deletion.finalize();
            stmt.finalize();
        });
    }
    res.redirect('/');
});
router.post('/conf/delete', function(req, res){
    if(req.param('key')!=null || req.param('key')==''){
        db.serialize(function() {
            var deletion = db.prepare("DELETE FROM tbl_cfg WHERE key=?");
            deletion.run(req.param('key'));
            deletion.finalize();
        });
    }
    res.redirect('/');
});

module.exports = router;
