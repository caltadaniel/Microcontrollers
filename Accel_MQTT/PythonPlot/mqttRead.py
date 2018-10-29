# -*- coding: utf-8 -*-
"""
Created on Tue Nov  7 20:39:14 2017

@author: calta
 """

#import context  # Ensures paho is in PYTHONPATH
import paho.mqtt.client as mqtt
import re
import sys
import time
from pyqtgraph.Qt import QtCore, QtGui
import numpy as np
import pyqtgraph as pg
from PyQt5.QtCore import QThread, pyqtSignal

class mqttComm(QThread):
    newFrame =pyqtSignal(str)
    def __init__(self):
        QThread.__init__(self)
        self.mqttc = mqtt.Client()
        self.mqttc.message_callback_add("/FFT/Accel1", self.on_message_msgs)
        self.mqttc.on_message = self.on_message
        self.mqttc.connect("192.168.20.40", 1883, 60)
        self.mqttc.subscribe("/FFT/#", 0)
        self.counter = 0
        self.execute = 1
        self.OutCycle = 0
        

    def __del__(self):
        self.abort()
        
    def on_message(self, mosq, obj, msg):
    # This callback will be called for messages that we receive that do not
    # match any patterns defined in topic specific callbacks, i.e. in this case
    # those messages that do not have topics $SYS/broker/messages/# nor
    # $SYS/broker/bytes/#
        print(msg.topic + " " + str(msg.qos) + " " + str(msg.payload))

    def on_message_msgs(self, mosq, obj, msg):
        # This callback will only be called for messages with topics that match
        # $SYS/broker/messages/#
        #print("MESSAGES: " + msg.topic + " " + str(msg.qos) + " " + str(msg.payload))
        if (str(msg.payload).find('Sending') == -1):
            
            #print (desired_array)
            
            self.newFrame.emit(str(msg.payload))
            self.counter = self.counter + 1
            print(self.counter)
            
    def abort(self):
        self.execute = 0
        while(self.OutCycle == 0):
            pass
    def run(self):
        while self.execute:
            self.mqttc.loop()
        self.OutCycle = 1



# Add message callbacks that will only trigger on a specific subscription match.


#
class App(QtGui.QMainWindow):

    
    
    def __init__(self, parent=None):
        super(App, self).__init__(parent)
        
        #### Create Gui Elements ###########
        self.mainbox = QtGui.QWidget()
        self.setCentralWidget(self.mainbox)
        self.mainbox.setLayout(QtGui.QVBoxLayout())

        self.canvas = pg.GraphicsLayoutWidget()
        self.mainbox.layout().addWidget(self.canvas)

        self.label = QtGui.QLabel()
        self.mainbox.layout().addWidget(self.label)

        self.view = self.canvas.addViewBox()
        self.view.setAspectLocked(True)
        self.view.setRange(QtCore.QRectF(0,0, 100, 100))

        #  image plot
        self.img = pg.ImageItem(border='w')
        self.view.addItem(self.img)

        self.canvas.nextRow()
        #  line plot
        self.otherplot = self.canvas.addPlot()
        self.h2 = self.otherplot.plot(pen='y')


        #### Set Data  #####################

        self.x = np.linspace(0,50., num=100)
        self.X,self.Y = np.meshgrid(self.x,self.x)

        self.counter = 0
        self.fps = 0.
        
        
        self.commThread = mqttComm()
        
        self.commThread.newFrame.connect(self._update);
        #self.newFrame.connect(self._update, SIGNAL("newFrame(QString)"), self.new_data)
        self.commThread.start()
        self.lastupdate = time.time()

        #### Start  #####################
        #self._update()

    def _update(self, newData):
        
        splittedPayload = newData.split(',')
        splittedPayload.pop(0)
        splittedPayload.pop(len(splittedPayload)-1)
        desired_array = [int(numeric_string) for numeric_string in splittedPayload]
        self.h2.setData(desired_array)

        now = time.time()
        dt = (now-self.lastupdate)
        if dt <= 0:
            dt = 0.000000000001
        fps2 = 1.0 / dt
        self.lastupdate = now
        self.fps = self.fps * 0.9 + fps2 * 0.1
        tx = 'Mean Frame Rate:  {fps:.3f} FPS'.format(fps=self.fps )
        self.label.setText(tx)
        self.counter += 1


if __name__ == '__main__':

    app = QtGui.QApplication(sys.argv)
    thisapp = App()
    thisapp.show()
    sys.exit(app.exec_())
