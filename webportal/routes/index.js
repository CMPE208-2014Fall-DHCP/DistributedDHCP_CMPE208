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
        KVs.push({"key": row.key, "val": row.val});
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

//FIXED IP
router.get('/fixedip', function(req, res){
    var context = [];
    var sql = "SELECT * FROM tbl_fixedip";
    db.each(sql, function(err, row){
        context.push({"hw_addr": row.hw_addr, "fixed_ip": IP_Utils.intToIP(row.fixed_ip)});
    }, function(){
        res.render('pages/fixedip', {
            title: 'DHCP fixed IP',
            ips: context
        })
    });
});
router.post('/fixedip', function(req, res){
    if(req.param('hw_addr')!=null || req.param('hw_addr')==''){
        db.serialize(function() {
            var deletion = db.prepare("DELETE FROM tbl_fixedip WHERE hw_addr=?");
            var stmt = db.prepare("INSERT INTO tbl_fixedip VALUES (? , ?)");
            deletion.run([req.param('hw_addr')]);
            stmt.run([req.param('hw_addr'), req.param('fixed_ip')]);
            deletion.finalize();
            stmt.finalize();
        });
    }
    res.redirect('/fixedip');
});
router.post('/fixedip/delete', function(req, res){
    if(req.param('hw_addr')!=null || req.param('hw_addr')==''){
        db.serialize(function() {
            var deletion = db.prepare("DELETE FROM tbl_fixedip WHERE hw_addr=?");
            deletion.run(req.param('hw_addr'));
            deletion.finalize();
        });
    }
    res.redirect('/fixedip');
});


module.exports = router;
