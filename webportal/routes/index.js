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
router.post('/conf', function(req, res){
    if(req.param('key')!=null || req.param('key')==''){
        console.log(req);
        db.serialize(function() {
            var deletion = db.prepare("DELETE FROM DHCP_CONF WHERE key=?");
            var stmt = db.prepare("INSERT INTO DHCP_CONF VALUES (? , ?)");
            deletion.run(req.param('key'));
            stmt.run([req.param('key'), req.param('value')]);

            stmt.finalize();
        });
    }
    res.redirect('/');
});

module.exports = router;
