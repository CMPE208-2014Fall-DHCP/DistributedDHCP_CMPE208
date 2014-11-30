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
        var ipArr = [0, 0, 0, 0];
        for (var i = 3; i >= 0; i--) {
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
        result += parseInt(ipArr[0]) * 256 * 256 * 256;
        result += parseInt(ipArr[1]) * 256 * 256;
        result += parseInt(ipArr[2]) * 256;
        result += parseInt(ipArr[3]);
        return result;
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
    // varify if a net mask is valid, '255.254.255.0' => false
    isValidMask: function(num) {
        var reverse = ~num;
        var sum = reverse + 1;
        return (reverse & sum) == 0;
    }
};
var ipUtilTest = function(){
    console.log('=========int to ip========');
    console.log(IP_Utils.intToIP(4294967295));
    console.log(IP_Utils.intToIP(294967295));
    console.log(IP_Utils.intToIP(3));

    console.log('=========ip to int========');
    console.log(IP_Utils.ipToInt('255.255.255.0'));//4294967040
    console.log(IP_Utils.ipToInt('172.17.1.10'));//2886795530
    console.log(IP_Utils.ipToInt('256.1.1.1'));//0

    console.log('=========ip is valid======');
    console.log(IP_Utils.isValidIP('-1.0.0.1'));
    console.log(IP_Utils.isValidIP('a.0.0.1'));
    console.log(IP_Utils.isValidIP('...'));

    console.log('=========Net mask is valid======');
    console.log(IP_Utils.isValidMask(IP_Utils.ipToInt('255.255.254.0')));//true
    console.log(IP_Utils.isValidMask(IP_Utils.ipToInt('255.254.255.0')));//false
    console.log(IP_Utils.isValidMask(4294967297));//false
}
ipUtilTest();
module.exports = IP_Utils;