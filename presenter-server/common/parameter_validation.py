"""Parameter Validation module"""
import logging

PORT_INTERVAL_BEGIN = 1024
PORT_INTERVAL_END = 49151


# Check IP validity
def validate_ip(ip_str):
    if ip_str == '0.0.0.0':
        logging.error("IP Addr \"0.0.0.0\" is illegal")
        print("IP Addr \"0.0.0.0\" is illegal")
        return False

    sep = ip_str.split('.')
    if len(sep) != 4:
        return False
    for i, x in enumerate(sep):
        try:
            int_x = int(x)
            if int_x < 0 or int_x > 255:
                logging.error("Illegal ip: %s", ip_str)
                print("Illegal ip: %s" % ip_str)
                return False
        except ValueError:
            logging.error("IP format error:%s", ip_str)
            print("IP format error:%s" % ip_str)
            return False
    return True


# Check Port validity
def validate_port(port_str):
    try:
        value = int(port_str)
        if value < PORT_INTERVAL_BEGIN or value > PORT_INTERVAL_END:
            logging.error("Illegal port: %d", value)
            print("Illegal port: %d" % value)
            return False
    except ValueError:
        logging.error("Port format error:%s", port_str)
        print("Port format error:%s" % port_str)
        return False
    return True


# Check int validity (begin,end]
def validate_integer(value_str, begin, end):
    try:
        value = int(value_str)
        if value <= begin or value > end:
            return False
    except ValueError:
        return False
    return True


# Check if str is greater than compared
def Integer_greater(value_str, compared_value):
    try:
        value = int(value_str)
        if value < compared_value:
            return False
    except ValueError:
        return False
    return True


# Check float validity [begin,end]
def validate_float(value_str, begin, end):
    try:
        value = float(value_str)
        if value <= begin or value > end:
            return False
    except ValueError:
        return False
    return True
