import time
import log
import checkNet
import utime
from aLiYun import aLiYun
from machine import UART
import log
import sms
from ucollections import deque
import ujson  # 导入ujson模块
from uio import StringIO

uart_log = log.getLogger("UART")
uart1 = UART(UART.UART1, 115200, 8, 0, 1, 0)

# 项目名称和版本
PROJECT_NAME = "QuecPython_AliYin_example"
PROJECT_VERSION = "1.0.0"
checknet = checkNet.CheckNetwork(PROJECT_NAME, PROJECT_VERSION)
uart1 = UART(UART.UART1, 115200, 8, 0, 1, 0)


# 设置日志输出级别
log.basicConfig(level=log.INFO)
aliYun_log = log.getLogger("ALiYun")

# 阿里云IoT平台认证参数
productKey = "k1uhsvTW4mz"       # 产品标识k1uhsvTW4mz
productSecret = "UhTjFGTw6cL5u55Y"  # 产品密钥UhTjFGTw6cL5u5
DeviceName = "EC600N"              # 设备名称
DeviceSecret = None  # 设备密钥，请替换为实际的设备密钥

state = 5
flag = 1

stored_phone_number = None
message_queue = deque((), 100)


def custom_zfill(s, width):
    """自定义 zfill 方法"""
    num_zeros = width - len(s)
    if num_zeros > 0:
        return '0' * num_zeros + s
    return s


# 回调函数
def sub_cb(topic, msg):
    global state, flag,stored_phone_number
    decoded_msg = msg.decode()
    aliYun_log.info("Subscribe Recv: Topic={}, Msg={}".format(topic.decode(), msg.decode()))
    
    current_time = utime.localtime()
    current_year = custom_zfill(str(current_time[0])[-2:], 2)
    current_month = custom_zfill(str(current_time[1]), 2)
    current_day = custom_zfill(str(current_time[2]), 2)
    current_hour = custom_zfill(str(current_time[3]), 2)

    if decoded_msg == "1000":
        message = "00{}{}{}{}0000000000".format(current_year, current_month, current_day, current_hour)
        uart1.write(message.encode())
        aliYun_log.info("Sent to serial: {}".format(message))
    elif decoded_msg == "1010":
        message = "00{}{}{}{}2222222222".format(current_year, current_month, current_day, current_hour)
        uart1.write(message.encode())
        aliYun_log.info("Sent to serial: {}".format(message))
    elif decoded_msg == "1011":
        message = "03{}{}{}{}0101010101".format(current_year, current_month, current_day, current_hour)
        uart1.write(message.encode())
        aliYun_log.info("Sent to serial: {}".format(message))
    elif decoded_msg == "1012":
        message = "03{}{}{}{}0202020202".format(current_year, current_month, current_day, current_hour)
        uart1.write(message.encode())
        aliYun_log.info("Sent to serial: {}".format(message))
    elif decoded_msg == "1013":
        message = "03{}{}{}{}0404040404".format(current_year, current_month, current_day, current_hour)
        uart1.write(message.encode())
        aliYun_log.info("Sent to serial: {}".format(message))
    elif decoded_msg == "1014":
        message = "03{}{}{}{}0303030303".format(current_year, current_month, current_day, current_hour)
        uart1.write(message.encode())
        aliYun_log.info("Sent to serial: {}".format(message))
    elif decoded_msg == "1015":
        flag=0
    elif decoded_msg == "1016":
        message = "88{}{}{}{}0000000000".format(current_year, current_month, current_day, current_hour)
        uart1.write(message.encode())
        aliYun_log.info("Sent to serial: {}".format(message))
    elif decoded_msg == "1017":
        message = "08{}{}{}{}0000000000".format(current_year, current_month, current_day, current_hour)
        uart1.write(message.encode())
        aliYun_log.info("Sent to serial: {}".format(message))
    elif len(decoded_msg) == 11:
        stored_phone_number = decoded_msg
        try:
            sms.sendTextMsg(stored_phone_number, '绑定成功', 'UCS2')
            aliYun_log.info("Sent SMS to {}: 绑定成功".format(stored_phone_number))
        except Exception as e:
            aliYun_log.error("Failed to send SMS to {}: {}".format(stored_phone_number, e))
    elif decoded_msg.startswith("11"):
        low = decoded_msg[2:6]
        high = decoded_msg[6:10]
        message = "02{}{}{}{}01{}{}".format(current_year, current_month, current_day, current_hour,low,high)
        uart1.write(message.encode())
        aliYun_log.info("Sent to serial: {}".format(message))
    elif decoded_msg.startswith("22"):
        low = decoded_msg[2:6]
        high = decoded_msg[6:10]
        message = "02{}{}{}{}02{}{}".format(current_year, current_month, current_day, current_hour,low,high)
        aliYun_log.info("Sent to serial: {}".format(message))
        aliYun_log.info("切片low值{}，high值{}".format(low,high))
        uart1.write(message.encode())
        aliYun_log.info("Sent to serial: {}".format(message))
    elif decoded_msg.startswith("33"):
        low = decoded_msg[2:6]
        high = decoded_msg[6:10]
        message = "02{}{}{}{}03{}{}".format(current_year, current_month, current_day, current_hour,low,high)
        uart1.write(message.encode())
        aliYun_log.info("Sent to serial: {}".format(message))
        aliYun_log.info("切片low值{}，high值{}".format(low,high))
    elif decoded_msg.startswith("44"):
        low = decoded_msg[2:6]
        high = decoded_msg[6:10]
        message = "02{}{}{}{}04{}{}".format(current_year, current_month, current_day, current_hour,low,high)
        uart1.write(message.encode())
        aliYun_log.info("Sent to serial: {}".format(message))
        

    state -= 1


if __name__ == '__main__':
    stagecode, subcode = checkNet.wait_network_connected(30)
    if stagecode == 3 and subcode == 1:
        aliYun_log.info('Network connection successful!')

        # 创建 aliyun 连接对象
        ali = aLiYun(productKey, productSecret, DeviceName, DeviceSecret)

        # 获取当前时间戳（毫秒）
        timestamp = int(time.time() * 1000)
        clientID = "k1uhsvTW4mz.EC600N"
        # 设置 mqtt 连接属性
        ali.setMqtt(clientID, clean_session=False, keepAlive=120)  # 设置较短的keepAlive以提高连接稳定性

        # 设置回调函数
        ali.setCallback(sub_cb)

        # 设置订阅和发布的主题
        sub_topic = "/k1uhsvTW4mz/EC600N/user/shou"  # 4G模块接收消息的主题
        pub_topic = "/k1uhsvTW4mz/EC600N/user/fa"    # 4G模块发布消息的主题

        # 尝试订阅主题
        try:
            ali.subscribe(sub_topic)
            aliYun_log.info("Subscribed to topic: {}".format(sub_topic))
        except Exception as e:
            aliYun_log.error("Subscribe failed with error: {}".format(e))

        # 开始运行
        ali.start()



        # 设置超时机制：如果 60 秒内没有收到消息或完成任务，退出循环
        timeout = time.time() + 7200  # 设置超时时间为 60 秒
        last_sent_hour = 0
        # 主循环：从串口读取消息并发布到话题
        while flag==1:
            if time.time() > timeout:
                aliYun_log.error("Timeout reached, exiting loop")
                break
            current_time = utime.localtime()

            current_year = custom_zfill(str(current_time[0])[-2:], 2) 
            current_month = custom_zfill(str(current_time[1]), 2)
            current_day = custom_zfill(str(current_time[4]), 2)   #    记得改回2    3是分钟     0  1  2  3  4  5
            current_hour = custom_zfill(str(current_time[5]), 2)#    记得改回3   4是秒		  年  月 日 时  分 秒
            hour = current_time[5]
            # 从每天 1 点开始，每隔 2 小时发送一次信息到串口
         #     if hour >= 1 and (hour - 1) % 2 == 0:
         #         if last_sent_hour != current_hour:
          #            message = "01{}{}{}{}1111111111".format(current_year, current_month, current_day, current_hour)
          #            try:
          #                uart1.write(message.encode())
          #                aliYun_log.info("Message sent to serial port.")
          #                last_sent_hour = current_hour
          #            except Exception as e:
           #               aliYun_log.error("Failed to send message to serial port: {}".format(e))  
            # 从每天 1 点开始，每隔 2 小时发送一次信息到串口
            if hour == 0 or hour  % 15 == 0:
                if last_sent_hour != current_hour: 
                    if hour==0:
                        Qflag="1"
                        flaggg = custom_zfill(Qflag, 2)
                    if hour==15:
                        Qflag="3"
                        flaggg = custom_zfill(Qflag, 2)
                    if hour==30:
                        Qflag="5"
                        flaggg = custom_zfill(Qflag, 2)
                    if hour==45:
                        Qflag="7"
                        flaggg = custom_zfill(Qflag, 2)
                    try:
                        message = "01{}{}{}{}9999999999".format(current_year, current_month, current_day, flaggg)
                        uart1.write(message.encode())
                        aliYun_log.info("Sent to serial：{}".format(message))
                        last_sent_hour = current_hour
                    except Exception as e:
                        aliYun_log.error("Failed to send message to serial port: {}".format(e))


            # 从串口读取消息
            if uart1.any():
                msg_from_uart_hasb = uart1.read()
                msg_from_uart = msg_from_uart_hasb.decode('utf-8')
                if msg_from_uart:
                    msg_from_uart = msg_from_uart.strip()
                    aliYun_log.info("串口接收到: {}".format(msg_from_uart))
                    # 将接收到的消息添加到队列中
                    message_queue.append(msg_from_uart)

            # 处理队列中的消息
            while message_queue:
                try:
                    msg = message_queue.popleft()
                    # 根据不同的消息类型进行处理
                    if msg.startswith("'newdata:"):
                        # 新数据，直接转发到小程序
                        ali.publish(pub_topic, msg)
                        aliYun_log.info("Message published to topic: {}".format(pub_topic))
                    elif msg.startswith("wl:"):
                        # 阈值数据，在信息最前面加上绑定的号码
                        if stored_phone_number:
                            new_msg = "'phone:{},{}'".format(stored_phone_number, msg)
                        else:
                            new_msg = "'phone:99999999999,{}'".format(msg)
                        ali.publish(pub_topic, new_msg)
                        aliYun_log.info("应该发送{}".format(new_msg))
                        aliYun_log.info("Message published to topic: {}".format(pub_topic))
                    elif msg.startswith("'时间:"):
                        # 历史记录，直接转发到小程序
                        ali.publish(pub_topic, msg)
                        aliYun_log.info("Message published to topic: {}".format(pub_topic))
                    elif msg.startswith("warning:"):
                        # 警报信息，不发布到小程序，发送短信
                        if stored_phone_number:
                            try:
                                UCS2MSG = msg.encode('UCS2')
                                sms.sendTextMsg(stored_phone_number, UCS2MSG, 'UCS2')
                                aliYun_log.info("Sent warning SMS to {}".format(stored_phone_number))
                            except Exception as e:
                                aliYun_log.error("Failed to send warning SMS to {}: {}".format(stored_phone_number, e))
                        else:
                            aliYun_log.info("未绑定号码")
                except Exception as e:
                    aliYun_log.error("Publish failed with error: {}".format(e))

            # 每次循环后等待0.2秒，降低负载
            time.sleep(0.2)

        # 退出程序时断开连接
        ali.disconnect()

    else:
        aliYun_log.info('Network connection failed! stagecode = {}, subcode = {}'.format(stagecode, subcode))
