var express = require('express');
var router = express.Router();
var db = require('./conn').db;

/* GET home page. */
router.get('/', function (req, res) {
    var KVs = [];
    var servers = [];
    var sql = "SELECT * FROM DHCP_CONF";
    db.each(sql, function (err, row) {
        KVs.push({"key": row.key, "value": row.value});
    }, function(){
        var sql2 = "SELECT * FROM active_tbl";
        db.each(sql2, function(err, row){
            servers.push({
                "owner": row.owner,
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
            var deletion = db.prepare("DELETE FROM DHCP_CONF WHERE key=?");
            var stmt = db.prepare("INSERT INTO DHCP_CONF VALUES (? , ?)");
            deletion.run(req.param('key'));
            stmt.run([req.param('key'), req.param('value')]);

            deletion.finalize();
            stmt.finalize();
        });
    }
    res.redirect('/');
});
router.post('/conf/delete', function(req, res){
    if(req.param('key')!=null || req.param('key')==''){
        db.serialize(function() {
            var deletion = db.prepare("DELETE FROM DHCP_CONF WHERE key=?");
            deletion.run(req.param('key'));
            deletion.finalize();
        });
    }
    res.redirect('/');
});

//FIXED IP
router.get('/fixedip', function(req, res){
    var context = [];
    var sql = "SELECT * FROM fixedip_tbl";
    db.each(sql, function(err, row){
        context.push({"identifer": row.identifer, "fixed_addr": row.fixed_addr});
    }, function(){
        res.render('pages/fixedip', {
            title: 'DHCP fixed IP',
            ips: context
        })
    });
});
router.post('/fixedip', function(req, res){
    if(req.param('identifer')!=null || req.param('identifer')==''){
        db.serialize(function() {
            var deletion = db.prepare("DELETE FROM fixedip_tbl WHERE identifer=? OR fixed_addr=?");
            var stmt = db.prepare("INSERT INTO fixedip_tbl VALUES (? , ?)");
            deletion.run([req.param('identifer'), req.param('fixed_addr')]);
            stmt.run([req.param('identifer'), req.param('fixed_addr')]);
            deletion.finalize();
            stmt.finalize();
        });
    }
    res.redirect('/fixedip');
});
router.post('/fixedip/delete', function(req, res){
    if(req.param('identifer')!=null || req.param('identifer')==''){
        db.serialize(function() {
            var deletion = db.prepare("DELETE FROM fixedip_tbl WHERE identifer=?");
            deletion.run(req.param('identifer'));
            deletion.finalize();
        });
    }
    res.redirect('/fixedip');
});


module.exports = router;
