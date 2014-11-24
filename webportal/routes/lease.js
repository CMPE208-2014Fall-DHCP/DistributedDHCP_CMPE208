/**
 * Created by leituo56 on 11/23/14.
 */
var express = require('express');
var router = express.Router();
var db = require('./conn').db;

router.get('/', function(req, res) {
    var context = [];
    db.each("SELECT * FROM lease_tbl", function(err, row){
        context.push({ "hw_addr" : row.hw_addr,
            "state": row.state,
            "timeout": row.timeout,
            "creator": row.creator,
            "owner": row.owner
        });
    }, function(){
        //res.json(context);
        res.render('pages/lease', {
            title: "DHCP Leases",
            leases: context
        })
    });
});

module.exports = router;