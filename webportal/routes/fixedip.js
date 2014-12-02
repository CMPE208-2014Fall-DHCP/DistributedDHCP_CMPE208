/**
 * Created by leituo56 on 11/29/14.
 */
var express = require('express');
var router = express.Router();
var db_cfg = require('./conn').db_cfg;
var IP_Utils = require('../public/javascripts/ip_util');

//FIXED IP
router.get('/', function(req, res){
    var context = [];
    var sql = "SELECT * FROM tbl_fixedip";
    db_cfg.each(sql, function(err, row){
        context.push({"hw_addr": row.hw_addr, "fixed_ip": IP_Utils.intToIP(row.fixed_ip)});
    }, function(){
        res.render('pages/fixedip', {
            title: 'DHCP fixed IP',
            ips: context
        })
    });
});
router.post('/', function(req, res){
    if(req.param('hw_addr')!=null || req.param('hw_addr')==''){
        db_cfg.serialize(function() {
            var deletion = db_cfg.prepare("DELETE FROM tbl_fixedip WHERE hw_addr=?");
            var stmt = db_cfg.prepare("INSERT INTO tbl_fixedip VALUES (? , ?)");
            deletion.run([req.param('hw_addr')]);
            stmt.run([req.param('hw_addr'), req.param('fixed_ip')]);
            deletion.finalize();
            stmt.finalize();
        });
    }
    res.redirect('/fixedip');
});
router.post('/delete', function(req, res){
    if(req.param('hw_addr')!=null || req.param('hw_addr')==''){
        db_cfg.serialize(function() {
            var deletion = db_cfg.prepare("DELETE FROM tbl_fixedip WHERE hw_addr=?");
            deletion.run(req.param('hw_addr'));
            deletion.finalize();
        });
    }
    res.redirect('/fixedip');
});

module.exports = router;