    // FileReceiveThread.cpp: implementation of the CFileReceiveThread class.   
    //   
    //////////////////////////////////////////////////////////////////////   
    // For compilers that support precompilation, includes "wx/wx.h".   
    #include "wx/wxprec.h"   
       
    #include <wx/wfstream.h>   
    #include <wx/sckstrm.h>   
    #include <wx/zstream.h>   
       
#include "tinyxml.h"  
#include "tcurrentUIDialog.h"
       
    //////////////////////////////////////////////////////////////////////   
    // Construction/Destruction   
    //////////////////////////////////////////////////////////////////////   
       
  void tcurrentUIDialog::CFileReceiveThread(wxString Filename, wxSocketBase* Socket)   
    {   
 
        // If we don't receive anything for 10 seconds, assume a timeout   
		m_Socket = Socket;
        m_Socket->SetTimeout(10);   
           
        // Wait for some data to come in, or for an error and block on the socket calls   
        m_Socket->SetFlags(wxSOCKET_WAITALL | wxSOCKET_BLOCK);   
       
        // Output to memory  		
       wxMemoryOutputStream FileOutputStream;   
        // Stream data in from the socket   
        wxSocketInputStream SocketInputStream(*m_Socket);   
        // The zlib decompression will decompress data from the socket stream   
        wxZlibInputStream ZlibInputStream(SocketInputStream);   
       
        // Write to the file stream the results of reading from the zlib input stream   
       FileOutputStream.Write(ZlibInputStream);   
		          
		int sz = FileOutputStream.GetSize();
        unsigned char* data = new unsigned char[sz];
		size_t numCopied = FileOutputStream.CopyTo(data, sz);

		if (numCopied != sz)
        {
			wxMessageBox(_T("not enough"));
          return;
        }
		
		FileOutputStream.Close();		
        m_Socket->Destroy(); 
		
		myData102 = data;		
		
    }  
