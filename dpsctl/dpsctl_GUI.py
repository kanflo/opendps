#!/usr/bin/env python

"""
#title           :dpsctl_GUI.py
#description     :dpsctl_GUI Is a simple GUI that makes calls to dpsctl.py
#author          :Spudmn
#date            :5/07/2018
#version         :0.1
#usage           :python dpsctl_GUI.py  
#notes           :
#python_version  :2.7.9  
#==============================================================================

#     This program is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, either version 3 of the License, or
#     (at your option) any later version.
# 
#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
# 
#     You should have received a copy of the GNU General Public License
#     along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""

import platform
import sys
import Tkinter as tk
import ttk as ttk
import subprocess


UDP_IP = "127.0.0.1"

 
class App(object):
    def __init__(self,parent):

        self.parent=parent
        self.parent.title("dpsctl GUI V1.00")
        
        self.Use_SCPI = False
        
        self.parent.geometry("800x400") #You want the size of the app to be 500x500
        self.parent.resizable(0, 0) #Don't allow resizing in the x or y direction
        
        
        self._setup_Volts_Frame_Widgets()  
        self._setup_Current_Frame_Widgets()   
        self._setup_Display_Frame_Widgets()
        self._setup_Ctrl_Frame_Widgets()
        
         
        #self._setup_Debug_Widgets()
        self.parent.after(250, self.On_Update_View_Timer) #Update the GUI       

    def _setup_Volts_Frame_Widgets(self):
    
        self.Volts_Disp_Frame = ttk.Labelframe(self.parent,text="Volts"
                                  ,relief=tk.GROOVE, borderwidth=1)
        
        self.Volts_Disp_Frame.pack(padx=10, pady=0,fill=tk.BOTH, expand=True)
        
        self.eb_Volts = ttk.Entry(self.Volts_Disp_Frame,font=('Arial', 24, 'bold'),width=5)
        self.eb_Volts.pack(padx=10, pady=5, side=tk.LEFT)
       
        
        self.sc_Volts = tk.Scale(self.Volts_Disp_Frame,from_=0.0, to=24.0,resolution=0.01,tickinterval=24/5, orient=tk.HORIZONTAL)
        self.sc_Volts.pack(padx=10, pady=5, side=tk.LEFT,fill=tk.X,expand=True)
        
        self.bt_Set_Volts = ttk.Button(self.Volts_Disp_Frame,text="Set Volts", command = self.On_bt_Set_Volts_Click)
        self.bt_Set_Volts.pack(padx=10, pady=10, side=tk.LEFT)
    
        
        
    def _setup_Current_Frame_Widgets(self):
    
        self.Current_Disp_Frame = ttk.Labelframe(self.parent,text="Current"
                                  ,relief=tk.GROOVE, borderwidth=1)
        
        self.Current_Disp_Frame.pack(padx=10, pady=0,fill=tk.BOTH, expand=True)
        
        self.eb_Current = ttk.Entry(self.Current_Disp_Frame,font=('Arial', 24, 'bold'),width=5)
        self.eb_Current.pack(padx=10, pady=5, side=tk.LEFT)
        
        
        self.sc_Current = tk.Scale(self.Current_Disp_Frame,from_=0, to=24,tickinterval=2, orient=tk.HORIZONTAL)
        self.sc_Current.pack(padx=10, pady=5, side=tk.LEFT,fill=tk.X,expand=True)

        self.bt_Set_Current = ttk.Button(self.Current_Disp_Frame,text="Set Current", command = self.On_bt_Set_Current_Click)
        self.bt_Set_Current.pack(padx=10, pady=10, side=tk.LEFT)

    def _setup_Display_Frame_Widgets(self):
    
        self.Display_Disp_Frame = ttk.Labelframe(self.parent,text="Display"
                                  ,relief=tk.GROOVE, borderwidth=1)
        
        self.Display_Disp_Frame.pack(padx=10, pady=0,fill=tk.BOTH, expand=True)
        
        self.Display_Disp_volts_In_Frame = ttk.Labelframe(self.Display_Disp_Frame,text="Volts In")
        self.Display_Disp_volts_In_Frame.pack(side=tk.LEFT,padx=10,)
        self.eb_Display_Volts_In = ttk.Entry(self.Display_Disp_volts_In_Frame,font=('Arial', 24, 'bold'),width=8)
        self.eb_Display_Volts_In.pack()
        
        self.Display_Disp_volts_Out_Frame = ttk.Labelframe(self.Display_Disp_Frame,text="Volts Out")
        self.Display_Disp_volts_Out_Frame.pack(side=tk.LEFT,padx=10,)
        self.eb_Display_Volts_Out = ttk.Entry(self.Display_Disp_volts_Out_Frame,font=('Arial', 24, 'bold'),width=8)
        self.eb_Display_Volts_Out.pack()
        
        self.Display_Disp_Current_Frame = ttk.Labelframe(self.Display_Disp_Frame,text="Current")
        self.Display_Disp_Current_Frame.pack(side=tk.LEFT,padx=10,)
        self.eb_Display_Current = ttk.Entry(self.Display_Disp_Current_Frame,font=('Arial', 24, 'bold'),width=8)
        self.eb_Display_Current.pack()
        
        
        self.Output_Enable_IntVar = tk.IntVar()
        self.rb_Output_On = ttk.Radiobutton(self.Display_Disp_Frame,text="Enable Output",variable=self.Output_Enable_IntVar,value=1)
        self.rb_Output_On.pack(side=tk.LEFT,padx=10)
        self.rb_Output_On.state(["disabled"])
        

    def _setup_Ctrl_Frame_Widgets(self):
    
        self.Ctrl_Frame = ttk.Labelframe(self.parent,text="Ctrl"
                                  ,relief=tk.GROOVE, borderwidth=1)
        
        self.Ctrl_Frame.pack(padx=10, pady=(0,10),fill=tk.BOTH, expand=True)

        
        self.bt_On = ttk.Button(self.Ctrl_Frame,text="ON", command = self.On_bt_On_Click)
        self.bt_On.pack(padx=10, pady=10, side=tk.RIGHT)
        self.Output_Enable_IntVar.set(1)
        
        self.bt_Off = ttk.Button(self.Ctrl_Frame,text="OFF", command = self.On_bt_Off_Click)
        self.bt_Off.pack(padx=10, pady=10, side=tk.RIGHT)
        self.Output_Enable_IntVar.set(0)
        
        self.bt_Query = ttk.Button(self.Ctrl_Frame,text="Query", command = self.On_bt_Query_Click)
        self.bt_Query.pack(padx=10, pady=10, side=tk.RIGHT) 
        
        self.bt_Ping = ttk.Button(self.Ctrl_Frame,text="Ping", command = self.On_bt_Ping_Click)
        self.bt_Ping.pack(padx=10, pady=10, side=tk.RIGHT)    
        
        self.lb_Status_String_Var = tk.StringVar()
        self.lb_Status = ttk.Label(self.Ctrl_Frame,textvariable=self.lb_Status_String_Var)
        self.lb_Status.pack(padx=20, pady=10, side=tk.LEFT)

   
        
    def on_sc_Volts_Changed(self,evt):
        self.eb_Volts.delete(0,tk.END)
        self.eb_Volts.insert(0,evt)
        pass
        
        
    def _setup_Debug_Widgets(self):
    
        self.Debug_Frame = ttk.Frame(self.parent,height=200,width=200
                                  ,relief=tk.GROOVE, borderwidth=1)
        
        self.Debug_Frame.pack(fill=tk.BOTH, expand=True,padx=5, pady=5)
        
        self.bt_Clear_Screen = ttk.Button(self.Debug_Frame,text="Debug 1", command = self.On_bt_Debug_1_Click)
        self.bt_Clear_Screen.pack(padx=10, pady=10, side=tk.RIGHT)
        
        self.bt_Insert = ttk.Button(self.Debug_Frame,text="Debug 2", command = self.On_bt_Debug_2_Click)
        self.bt_Insert.pack(padx=10, pady=10, side=tk.RIGHT)
        
    def On_bt_Debug_1_Click(self):
        pass


    def On_bt_Debug_2_Click(self):
        pass
    
    
    
    def Open_UDP_Port(self):
        pass
    
    def Close_UDP_Port(self):
        pass
        
        
    def Run_and_Get_dpsctl(self,Args):

        cmd = ["python", "dpsctl.py", "-d" , UDP_IP]
        cmd += Args
        if platform.system() == 'Linux':
            #For some reason on Linux. When using Popen with shell=True
            #you need to join the command line into a string 
            cmd_str = ""    
            for words in cmd:
                cmd_str += words + " "
            cmd = [cmd_str]

        process = subprocess.Popen(cmd, shell=True,
                           stdout=subprocess.PIPE, 
                           stderr=subprocess.PIPE)
        stdout_pipe, stderr_pipe = process.communicate()
        errcode = process.returncode
        return errcode,stdout_pipe,stderr_pipe
        
        
    def Run_and_Get_Command(self,Args):
        if self.Use_SCPI:
            print "To Do"
        else:
            errcode,stdout_pipe,_stderr_pipe =self.Run_and_Get_dpsctl(Args)
            if (errcode != 0):
                self.lb_Status_String_Var.set(stdout_pipe)
            else:
                self.lb_Status_String_Var.set("")
            return errcode,stdout_pipe,_stderr_pipe
 
            
    
    def On_bt_On_Click(self):
        print self.Run_and_Get_Command(["--enable", "on"])
        
        
    def On_bt_Off_Click(self):
        print self.Run_and_Get_Command(["--enable", "off"])
        
    def On_bt_Ping_Click(self):
        print self.Run_and_Get_Command(["--ping"])
        
      
    def On_bt_Query_Click(self):
        errcode,stdout_pipe,_stderr_pipe = self.Run_and_Get_Command(["-q"])
        if errcode == 0:
            print stdout_pipe
            lines = stdout_pipe.split("\n")
            Info_Dict = {}
            for line in lines:
                line_split = line.split(":")
                if (len(line_split) == 2):
                    key = line_split[0].strip()
                    value = line_split[1].strip()
                    Info_Dict.update({key:value})
                    
            print Info_Dict
            self.eb_Volts.delete(0,tk.END)
            self.eb_Volts.insert(0,Info_Dict["voltage"])
            
            self.eb_Current.delete(0,tk.END)
            self.eb_Current.insert(0,Info_Dict["current"])
            
            self.eb_Display_Volts_In.delete(0,tk.END)
            self.eb_Display_Volts_In.insert(0,Info_Dict["V_in"])
            
            self.eb_Display_Volts_Out.delete(0,tk.END)
            self.eb_Display_Volts_Out.insert(0,Info_Dict["V_out"])
     
            self.eb_Display_Current.delete(0,tk.END)
            self.eb_Display_Current.insert(0,Info_Dict["I_out"])
            
            if "on" in Info_Dict["Func"]:
                self.Output_Enable_IntVar.set(1)
            if "off" in Info_Dict["Func"]:
                self.Output_Enable_IntVar.set(0)

        
        
    def On_bt_Set_Volts_Click(self):
        print self.Run_and_Get_Command(["-p voltage=" + str(self.sc_Volts.get())])
        pass
    
    def On_bt_Set_Current_Click(self):
        print self.Run_and_Get_Command(["-p current="+ str(self.sc_Current.get())])
    
    
    def On_Update_View_Timer(self):
        self.On_bt_Query_Click()
        self.parent.after(250, self.On_Update_View_Timer) #Update the GUI
    

def main(_argv):
    root = tk.Tk()
    _app = App(root)
    root.mainloop()



if __name__ == "__main__":
    main(sys.argv[1:])
