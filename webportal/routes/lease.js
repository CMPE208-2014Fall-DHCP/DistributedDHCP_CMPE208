/**
 * Created by leituo56 on 11/23/14.
 */
var express = require('express');
var router = express.Router();
var db_lease = require('./conn').db_lease;
var IP_Utils = require('../public/javascripts/ip_util');

router.get('/', function(req, res) {
    var scope = 'All';
    var context = [];
    var sql = "SELECT * FROM tbl_lease";
    if(req.param('owner')!=null){
        var owner = req.param('owner');
        sql += " WHERE owner='" + owner + "' and state > 0";
        scope = IP_Utils.intToIP(owner);
    }else{
        sql += " WHERE state > 0";
    }
    db_lease.each(sql, function(err, row){
        context.push({
            "lease_ip": IP_Utils.intToIP(row.lease_ip),
            "hw_addr" : row.hw_addr,
            "state": row.state,
            "timeout": new Date(row.timeout * 1000),
            "creator": row.creator,
            "owner": IP_Utils.intToIP(row.owner)
        });
    }, function(){
        //res.json(context);
        res.render('pages/lease', {
            title: "DHCP Leases [" + scope + ']',
            leases: context
        });
    });
});

module.exports = router;