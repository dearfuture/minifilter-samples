VERSION 5.00
Begin VB.Form Form1 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "TestMiniFilter - Powered by tangptr@126.com"
   ClientHeight    =   1680
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   9465
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   1680
   ScaleWidth      =   9465
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Command1 
      Caption         =   "Protect"
      Height          =   735
      Left            =   120
      TabIndex        =   2
      Top             =   840
      Width           =   9255
   End
   Begin VB.Frame Frame1 
      Caption         =   "Input File Name - Please do not input the full path"
      Height          =   615
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   9255
      Begin VB.TextBox Text1 
         Height          =   285
         Left            =   120
         TabIndex        =   1
         Text            =   "fucker.txt"
         Top             =   240
         Width           =   9015
      End
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Dim DrvCtrl As New cls_Driver

Private Sub Command1_Click()
Dim pBuff() As Integer
Dim pLen As Long
Dim i As Long
pLen = Len(Text1.Text)
ReDim pBuff(1 To pLen)
For i = 1 To pLen Step 1
    pBuff(i) = AscW(Mid(Text1.Text, i, 1))
Next i
DrvCtrl.IoControl DrvCtrl.CTL_CODE_GEN(&H801), VarPtr(pBuff(1)), pLen * 2, 0, 0
End Sub

Private Sub Form_Load()
With DrvCtrl
    .szDrvFilePath = Replace(App.Path & "\driver.sys", "\\", "\")
    .szDrvDisplayName = "TestMiniFilter"
    .szDrvLinkName = "TestMiniFilter"
    .szDrvSvcName = "TestMiniFilter"
    .InstDrv
    .StartDrv
    If .OpenDrv = False Then MsgBox "Failed to load driver", vbExclamation, "Error": End
End With
End Sub

Private Sub Form_Unload(Cancel As Integer)
With DrvCtrl
    .StopDrv
    .DelDrv
End With
End Sub
