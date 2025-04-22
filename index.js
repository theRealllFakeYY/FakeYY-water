const mqtt = require('../../lib/mqtt.min.js');

Page({
  data: {
    A: '',
    B: '',
    C: '',
    D: '',
    E: '',
    F: '',
    PlowValue: 6.5,
    PhighValue: 8.5,
    DlowValue: 0.1,
    DhighValue: 1.2,
    RlowValue: 0,
    RhighValue: 1.0,
    WlowValue: 0,
    WhighValue: 1.5,
    phoneValue:'',
    inputPlow: 6.5,
    inputPhigh: 8.5,
    inputDlow: 0.1,
    inputDhigh: 1.2,
    inputRlow: 0,
    inputRhigh: 1.0,
    inputWlow: 0,
    inputWhigh: 1.5,
    inputphone:'',
    pColorClass: '',
    dColorClass: '',
    rColorClass: '',
    wColorClass: '',
    tableData: [],  // 使用与wxml一致的变量名
    showTableModal: false,
    tableTitle: '',
    messageQueue: [],
    isSending: false
  },

  onLoad() {
    
    
    wx.showLoading({
      title: '连接中...',
    });


    // 创建MQTT客户端，传入连接参数
    const client = mqtt.connect('wxs://iot-06z00jdfem1zp2p.mqtt.iothub.aliyuncs.com', {
      clientId: 'k1uhsvTW4mz.YINGYING|securemode=2,signmethod=hmacsha256,timestamp=1731135160723|',
      username: 'YINGYING&k1uhsvTW4mz',
      password: 'b225ddcbd0acfd4c9550958869e6543f8f0931149fd5be9402f68575e5c88224',
    });

    // 连接成功后执行
    client.on('connect', () => {
      console.log('连接成功');
      this.setData({ client });

      // 订阅4G模块发送消息给小程序的主题
      client.subscribe('/k1uhsvTW4mz/YINGYING/user/shou', (err) => {
        if (err) {
          console.error('订阅失败:', err);
          wx.showToast({
            title: '订阅失败',
            icon: 'none'
          });
        } else {
          console.log('订阅成功:/k1uhsvTW4mz/YINGYING/user/shou');
        }
      });

      // 接收到消息时执行，添加处理逻辑来更新A、B、C、D、参数
      client.on('message', (topic, message) => {
      console.log('Raw message:', message);
      console.log('Decoded message:', message.toString());
      console.log('收到消息:', topic, message.toString());
        
        const messageStr = message.toString();



 if (messageStr.startsWith('newdata:')) {
          const dataPart = messageStr.slice(8);
  // 按逗号分隔
          const parts = dataPart.split(',');
          const data = {};
          parts.forEach(part => {
            const [key, value] = part.split(':');
            data[key] = value;
          });
          this.setData({
            A: parseFloat(data.p),
            B: parseFloat(data.d),
            C: parseFloat(data.r),
            D: parseFloat(data.w)
          });
          wx.showToast({
            title: '已更新',
            icon: 'none'
          })

        // 数据更新后调用updateColorClasses函数来更新颜色类名
        this.updateColorClasses();
      }


      if (messageStr && messageStr.length > 10) {
        console.log('Raw message:', messageStr); // 输出原始消息内容
        
        // 按逗号拆分字符串，跳过空字符串
        const parts = messageStr.split(',').filter(part => part.trim() !== '');
      
        console.log('Split parts:', parts); // 输出拆分后的部分，确认格式
      
        // 格式化数据为表格所需格式
        const formattedData = [];
      
        if (parts[0]&&parts[0].length ==17){ //处理初始化   
          console.log('1部分:', parts[0]); // 输出原始消息内容
          console.log('1部分长度:', parts[0].length); // 输出原始消息内容
          let phoneNumber = parts[0].split(':')[1]?.trim();

          if (phoneNumber) {
              phoneNumber = phoneNumber.slice(-4);
          }
          if (phoneNumber!=9999){
          this.setData({
            phoneValue: phoneNumber,
            WlowValue: parts[1].split(':')[1]?.trim(),
            WhighValue: parts[2].split(':')[1]?.trim(),
            PlowValue: parts[3].split(':')[1]?.trim(),
            PhighValue: parts[4].split(':')[1]?.trim(),
            DlowValue: parts[5].split(':')[1]?.trim(),
            DhighValue: parts[6].split(':')[1]?.trim(),
            RlowValue: parts[7].split(':')[1]?.trim(),
            RhighValue: parts[8].split(':')[1]?.trim(),
            isLoading:false
          },() =>{
            // 在数据更新完成后关闭加载框
            wx.hideLoading();
        } 

          );}

          if (phoneNumber==9999){
            this.setData({
              WlowValue: parts[1].split(':')[1]?.trim(),
              WhighValue: parts[2].split(':')[1]?.trim(),
              PlowValue: parts[3].split(':')[1]?.trim(),
              PhighValue: parts[4].split(':')[1]?.trim(),
              DlowValue: parts[5].split(':')[1]?.trim(),
              DhighValue: parts[6].split(':')[1]?.trim(),
              RlowValue: parts[7].split(':')[1]?.trim(),
              RhighValue: parts[8].split(':')[1]?.trim(),
              isLoading:false
            },() =>{
              // 在数据更新完成后关闭加载框
              wx.hideLoading();
              wx.showModal({
                title: '暂未绑定手机号，请尽快绑定',
                showCancel: false, // 不显示取消按钮
                confirmText: 'OK', // 自定义确认按钮文字
            });
          } );
        }
      }

      const newFormatRegex = /时间:\d+,MAX:\d+,MIN:\d+,SUM:\d+/g;
      const newFormatMatches = messageStr.match(newFormatRegex);
      if (newFormatMatches) {
        // 创建临时数组保存现有数据
        let updatedTableData = this.data.tableData.slice();
        
        newFormatMatches.forEach(match => {
          const parts = match.split(',');
          const time = parts[0].split(':')[1];
          const max = parts[1].split(':')[1];
          const min = parts[2].split(':')[1];
          const sum = parts[3].split(':')[1];
          
          // 添加新数据到数组
          updatedTableData.push({
            time: time.padStart(6, '0'),  // 格式化为6位时间码
            max: parseInt(max).toFixed(1),
            min: parseInt(min).toFixed(1),
            sum: parseInt(sum).toFixed(1)
          });
        });
    
        // 更新数据并保持弹窗显示
        this.setData({
          tableData: updatedTableData,
          showTableModal: true
        });
      }

      
   } 
});
      this.sendMessage('1000');
     
      setInterval(() => {
        this.sendMessage('1010');
        
      }, 9  * 1000);
      
      });

    // 连接失败时执行
    client.on('error', (err) => {
      console.error('连接失败:', err);
      wx.showToast({
        title: '连接失败',
        icon: 'none'
      });
    });
  },

  onInputPlow: function (e) {
    this.setData({
      inputPlow: parseFloat(e.detail.value)
    });
  },

  onInputPhigh: function (e) {
    this.setData({
      inputPhigh: parseFloat(e.detail.value)
    });
  },

  onInputDlow: function (e) {
    this.setData({
      inputDlow: parseFloat(e.detail.value)

    });
  },

  onInputDhigh: function (e) {
    this.setData({
      inputDhigh: parseFloat(e.detail.value)
    });
  },

  onInputRlow: function (e) {
    this.setData({
      inputRlow: parseFloat(e.detail.value)
    });
  },

  onInputRhigh: function (e) {
    this.setData({
      inputRhigh: parseFloat(e.detail.value)
    });
  },

  onInputWlow: function (e) {
    this.setData({
      inputWlow: parseFloat(e.detail.value)
    });
  },

  onInputWhigh: function (e) {
    this.setData({
      inputWhigh: parseFloat(e.detail.value)
    });
  },
  
  onInputphone: function (e) {
    this.setData({
      inputphone: e.detail.value
    });
  },

  break: function () {
    this.sendMessage('1015');
    wx.showToast({
      title: '打断！',
      icon: 'none'
    });
  },
 
  confirmRangephone: function () {
    const phoneRegex = /^1[3-9]\d{9}$/;
    const inputPhone = this.data.inputphone.toString();
    if (!phoneRegex.test(inputPhone)) {
      wx.showToast({
        title: '请输入有效的11位手机号',
        icon: 'none'
      });
      return;
    }

    else {
      wx.showToast({
          title: '绑定中...',
      });
  }

    
    this.sendMessage(inputPhone);
    if (this.data.inputphone!== undefined) {
      this.setData({
        phoneValue: this.data.inputphone
      });
    }

   
  },

//1
confirmRangeP: function () {
  if (this.data.inputPlow > this.data.inputPhigh) {
      wx.showModal({
          title: '左输入阈值下限,右输入阈值上限,请重新输入',
          showCancel: false, // 不显示取消按钮
          confirmText: 'OK', // 自定义确认按钮文字
      });
      return;
  }
  if (this.data.inputPlow!== undefined) {
      this.setData({
          PlowValue: this.data.inputPlow
      });
  }
  if (this.data.inputPhigh!== undefined) {
      this.setData({
          PhighValue: this.data.inputPhigh
      });
  }
  wx.showModal({
      title: '修改ph阈值成功',
      showCancel: false, // 不显示取消按钮
      confirmText: 'OK', // 自定义确认按钮文字
  });
  // 范围值更新后调用updateColorClasses函数来更新颜色类名
  this.updateColorClasses();

  // 处理PlowValue和PhighValue为4位字符串格式
  const formattedPlow = String(this.data.PlowValue).padStart(4, '0');
  const formattedPhigh = String(this.data.PhighValue).padStart(4, '0');
  const message = `11${formattedPlow}${formattedPhigh}`;
  this.sendMessage(message);
},
// 3
confirmRangeW: function () {
  if (this.data.inputWlow > this.data.inputWhigh) {
      wx.showModal({
          title: '左输入阈值下限,右输入阈值上限,请重新输入',
          showCancel: false, // 不显示取消按钮
          confirmText: 'OK', // 自定义确认按钮文字
      });
      return;
  }
  if (this.data.inputWlow!== undefined) {
      this.setData({
          WlowValue: this.data.inputWlow
      });
  }
  if (this.data.inputWhigh!== undefined) {
      this.setData({
          WhighValue: this.data.inputWhigh
      });
  }
  wx.showModal({
      title: '修改污浊度阈值成功',//'请先输入阈值下限，请重新输入',
      showCancel: false, // 不显示取消按钮
      confirmText: 'OK', // 自定义确认按钮文字
  });
  // 范围值更新后调用updateColorClasses函数来更新颜色类名
  this.updateColorClasses();

  const formattedWlow = String(this.data.WlowValue).padStart(4, '0');
  const formattedWhigh = String(this.data.WhighValue).padStart(4, '0');
  const message = `33${formattedWlow}${formattedWhigh}`;
  this.sendMessage(message);
},
// 2
confirmRangeR: function () {
  if (this.data.inputRlow > this.data.inputRhigh) {
      wx.showModal({
          title: '左输入阈值下限,右输入阈值上限,请重新输入',
          showCancel: false, // 不显示取消按钮
          confirmText: 'OK', // 自定义确认按钮文字
      });
      return;
  }
  if (this.data.inputRlow!== undefined) {
      this.setData({
          RlowValue: this.data.inputRlow
      });
  }
  if (this.data.inputRhigh!== undefined) {
      this.setData({
          RhighValue: this.data.inputRhigh
      });
  }
  wx.showModal({
      title: '修改溶解度阈值成功',
      showCancel: false, // 不显示取消按钮
      confirmText: 'OK', // 自定义确认按钮文字
  });
  // 范围值更新后调用updateColorClasses函数来更新颜色类名
  this.updateColorClasses();

  const formattedRlow = String(this.data.RlowValue).padStart(4, '0');
  const formattedRhigh = String(this.data.RhighValue).padStart(4, '0');
  const message = `22${formattedRlow}${formattedRhigh}`;
  this.sendMessage(message);
},
// 4
confirmRangeD: function () {
  if (this.data.inputDlow > this.data.inputDhigh) {
      wx.showModal({
          title: '左输入阈值下限,右输入阈值上限,请重新输入',
          showCancel: false, // 不显示取消按钮
          confirmText: 'OK', // 自定义确认按钮文字
      });
      return;
  }
  if (this.data.inputDlow!== undefined) {
      this.setData({
          DlowValue: this.data.inputDlow
      });
  }
  if (this.data.inputDhigh!== undefined) {
      this.setData({
          DhighValue: this.data.inputDhigh
      });
  }
  wx.showModal({
      title: '修改电导率阈值成功',
      showCancel: false, // 不显示取消按钮
      confirmText: 'OK', // 自定义确认按钮文字
  });
  // 范围值更新后调用updateColorClasses函数来更新颜色类名
  this.updateColorClasses();

  const formattedDlow = String(this.data.DlowValue).padStart(4, '0');
  const formattedDhigh = String(this.data.DhighValue).padStart(4, '0');
  const message = `44${formattedDlow}${formattedDhigh}`;
  this.sendMessage(message);
},



  
  viewHistoryPH: function () {
    this.setData({ tableData: [] });
    this.sendMessage('1011');
    wx.showLoading({
        title: '获取中...',
    });
    setTimeout(() => {
        wx.hideLoading();
        this.setData({
            showTableModal: true, // 显示弹窗
            tableTitle: 'PH历史数据' // 设置弹窗标题
        });
    }, 1800);
}, //PH历史

viewHistorySolubility: function () {
  this.setData({ tableData: [] });
    this.sendMessage('1012');
    wx.showLoading({
        title: '获取中...',
    });
    setTimeout(() => {
        wx.hideLoading();
        this.setData({
            showTableModal: true, // 显示弹窗
            tableTitle: '溶解度历史数据' // 设置弹窗标题
        });
    }, 1800);
}, //溶解度历史

viewHistoryConductivity: function () {
  this.setData({ tableData: [] });
    this.sendMessage('1013');
    wx.showLoading({
        title: '获取中...',
    });
    setTimeout(() => {
        wx.hideLoading();
        this.setData({
            showTableModal: true, // 显示弹窗
            tableTitle: '电导率历史数据' // 设置弹窗标题
        });
    }, 1800);
}, //电导率历史

viewHistoryTurbidity: function () {
  this.setData({ tableData: [] });
    this.sendMessage('1014');
    wx.showLoading({
        title: '获取中...',
    });
    setTimeout(() => {
        wx.hideLoading();
        this.setData({
            showTableModal: true, // 显示弹窗
            tableTitle: '污浊度历史数据' // 设置弹窗标题
        });
    }, 1800);
}, //污浊度历史    


openled: function () {
  this.sendMessage('1016');
},

closeled: function () {
  this.sendMessage('1017');
},

sendMessage: function (message) {
  const { client, messageQueue, isSending } = this.data;
  
  // 将新消息加入队列
  messageQueue.push(message);
  
  // 如果没有在发送中，立即开始处理队列
  if (!isSending) {
    this.processQueue();
  }
},

// 新增队列处理方法
processQueue: function () {
  const { client, messageQueue } = this.data;
  
  if (messageQueue.length === 0) {
    this.setData({ isSending: false });
    return;
  }

  this.setData({ isSending: true });
  
  // 取出队列第一条消息
  const message = messageQueue.shift();
  
  // 实际发送逻辑
  if (!client) {
    wx.showToast({ title: 'MQTT未连接', icon: 'none' });
    return;
  }

  client.publish('/k1uhsvTW4mz/YINGYING/user/fa', message, {}, (err) => {
    if (err) {
      console.error('发送失败:', err);
      wx.showToast({ title: '发送失败', icon: 'none' });
    }
    
    // 每秒处理下一条（1000ms）
    setTimeout(() => {
      this.processQueue();
    }, 1000);
  });
},








  closeTableModal() {
    this.setData({
      showTableModal: false
    });
  },

  updateColorClasses: function () {
    let {A, B, C, D, PlowValue, PhighValue, DlowValue, DhighValue, RlowValue, RhighValue, WlowValue, WhighValue} = this.data;

    // 判断PH值的颜色
    if (A <= PlowValue || A >= PhighValue) {
      this.setData({pColorClass: 'red-text'});
    } else if (Math.abs(A - PlowValue) <= (PhighValue - PlowValue) * 0.1 || Math.abs(A - PhighValue) <= (PhighValue - PlowValue) * 0.1) { 
      this.setData({pColorClass: 'yellow-text'});
    } else {
      this.setData({pColorClass: 'green-text'});
    }

    // 判断电导率值的颜色
    if (B <= DlowValue || B >= DhighValue) {
      this.setData({dColorClass: 'red-text'});
    } else if (Math.abs(B - DlowValue) <= (DhighValue - DlowValue) * 0.1 || Math.abs(B - DhighValue) <= (DhighValue - DlowValue) * 0.1) { 
      this.setData({dColorClass: 'yellow-text'});
    } else {
      this.setData({dColorClass: 'green-text'});
    }

    // 判断固体溶解度值的颜色
    if (C <= RlowValue || C >= RhighValue) {
      this.setData({rColorClass: 'red-text'});
    } else if (Math.abs(C - RlowValue) <= (RhighValue - RlowValue) * 0.1 || Math.abs(C - RhighValue) <= (RhighValue - RlowValue) * 0.1) { 
      this.setData({rColorClass: 'yellow-text'});
    } else {
      this.setData({rColorClass: 'green-text'});
    }

    // 判断污浊度值的颜色
    if (D <= WlowValue || D >= WhighValue) {
      this.setData({wColorClass: 'red-text'});
    } else if (Math.abs(D - WlowValue) <= (WhighValue - WlowValue) * 0.1 || Math.abs(D - WhighValue) <= (WhighValue - WlowValue) * 0.1) { 
      this.setData({wColorClass: 'yellow-text'});
    } else {
      this.setData({wColorClass: 'green-text'});
    }

    this.setData({
      pColorClass: this.data.pColorClass,
      dColorClass: this.data.dColorClass,
      rColorClass: this.data.rColorClass,
      wColorClass: this.data.wColorClass
    });
  },

})









