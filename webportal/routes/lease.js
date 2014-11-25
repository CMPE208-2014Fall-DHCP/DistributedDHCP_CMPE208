/**
 * Created by leituo56 on 11/23/14.
 */
var express = require('express');
var router = express.Router();
var db = require('./conn').db;

router.get('/', function(req, res) {
    var context = [];
    var sql = "SELECT lease_tbl.*, fixedip_tbl.fixed_addr FROM lease_tbl LEFT JOIN fixedip_tbl ON lease_tbl.hw_addr=fixedip_tbl.identifer";
    if(req.param('owner')!=null){
        var owner = req.param('owner');
        sql += " WHERE owner='" + owner + "'";
    }
    db.each(sql, function(err, row){
        context.push({ "hw_addr" : row.hw_addr,
            "state": row.state,
            "timeout": row.timeout,
            "creator": row.creator,
            "owner": row.owner,
            "ip": row.fixed_addr
        });
    }, function(){
        //res.json(context);
        res.render('pages/lease', {
            title: "DHCP Leases",
            leases: context
        });
    });
});

module.exports = router;