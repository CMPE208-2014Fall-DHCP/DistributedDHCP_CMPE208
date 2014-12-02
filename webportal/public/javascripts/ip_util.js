/**
 * Created by leituo56 on 11/27/14.
 */
var IP_Utils;
IP_Utils = {
    // IP limits => unsigned int 32
    MAX: 4294967295,
    MIN: 0,
    // Number => IP address, e.g. 4294967295=>'255.255.255.255'
    intToIP: function (num) {
        if(num < 0)
            num = num >>> 0;//change num to unsigned if < 0
        var ipArr = [0, 0, 0, 0];
        for (var i = 0; i < 4; i++) {
            ipArr[i] = num % 256;
            num = Math.floor(num / 256);
        }
        return ipArr.join(".");
    },
    // IP address => Number, e.g. '255.255.255.255' => 4294967295
    ipToInt: function (ip) {
        if(!this.isValidIP(ip))
            return 0;
        var ipArr = ip.split('.');
        var result = 0;
        result += parseInt(ipArr[3]) * 256 * 256 * 256;
        result += parseInt(ipArr[2]) * 256 * 256;
        result += parseInt(ipArr[1]) * 256;
        result += parseInt(ipArr[0]);
        return (result >> 0);
    },
    // varify if an ip is valid, e.g. '-1.0.0.1' => false
    isValidIP: function (ip) {
        var ipArr = ip.split('.');
        if(ipArr.length != 4)
            return false;
        for(var i=0;i<4;i++){
            var test = parseInt(ipArr[i]);
            if(isNaN(test) || test < 0 || test > 255)
                return false;
        }
        return true;
    },
    // change the Endianness of a number
    changeEndian: function(val) {
        return ((val & 0xFF) << 24)
            | ((val & 0xFF00) << 8)
            | ((val >> 8) & 0xFF00)
            | ((val >> 24) & 0xFF);
    },
    // verify if a net mask is valid, ipToInt('255.254.255.0') => false
    isValidMask: function(num) {
        num = this.changeEndian(num);
        var reverse = ~num;//11111000 => 00000111
        var sum = reverse + 1;//00000111 => 00001000
        return (reverse & sum) == 0;//00000111 & 00001000==0
    },
    isValidSubnet: function(s, e, m, r){
        var start = this.ipToInt(s);
        var end = this.ipToInt(e);
        var mask = this.ipToInt(m);
        var router = this.ipToInt(r);
        var net = router & mask;
        return (start & mask) == net && (end & mask) == net;
    }
};
var ipUtilTest = function(){
    console.log('=========int to ip========');
    console.log(IP_Utils.intToIP(16777215));
    console.log(IP_Utils.intToIP(509913280));
    console.log(IP_Utils.intToIP(-2107332416));

    console.log('=========ip to int========');
    console.log(IP_Utils.ipToInt('255.255.255.0'));//4294967040
    console.log(IP_Utils.ipToInt('192.168.100.130'));//2886795530
    console.log(IP_Utils.ipToInt('256.1.1.1'));//0

    console.log('=========ip is valid======');
    console.log(IP_Utils.isValidIP('-1.0.0.1'));
    console.log(IP_Utils.isValidIP('a.0.0.1'));
    console.log(IP_Utils.isValidIP('...'));

    console.log('=========Net mask is valid======');
    console.log(IP_Utils.isValidMask(IP_Utils.ipToInt('255.255.254.0')));//true
    console.log(IP_Utils.isValidMask(IP_Utils.ipToInt('255.254.255.0')));//false
    console.log(IP_Utils.isValidMask(16777215));//true

    console.log('=========Subnet is valid======');
    console.log(IP_Utils.isValidSubnet('172.17.1.10', '172.17.1.50', '255.255.255.0', '172.17.1.1'));
    console.log(IP_Utils.isValidSubnet('172.17.1.10', '172.17.2.50', '255.255.255.0', '172.17.1.1'));
}
//ipUtilTest();
try {
    module.exports = IP_Utils;
}catch (e){
    console.log('client');
}