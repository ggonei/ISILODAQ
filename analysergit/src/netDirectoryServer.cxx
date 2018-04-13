/********************************************************************\

  Name:         netDirectoryServer.cxx
  Created by:   Konstantin Olchanski & Stefan Ritt

  Contents:     Server component for the ROOT TNetDirectory

  $Id$

\********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "netDirectoryServer.h"

/*==== ROOT socket histo server ====================================*/

#if 1
#define THREADRETURN
#define THREADTYPE void
#endif
#if defined( OS_WINNT )
#define THREADRETURN 0
#define THREADTYPE DWORD WINAPI
#endif
#ifndef THREADTYPE
#define THREADTYPE int
#define THREADRETURN 0
#endif

#include <TROOT.h>
#include <TClass.h>
#include <TFile.h>
#include <TDirectory.h>
#include <TKey.h>
#include <TFolder.h>
#include <TSocket.h>
#include <TServerSocket.h>
#include <TThread.h>
#include <TMessage.h>
#include <TObjString.h>
#include <TH1.h>
#include <TCutG.h>

#include <deque>
#include <map>
#include <string>

#include "RootLock.h"

static bool gVerbose = false;

static std::deque<std::string> gExports;
static std::map<std::string,std::string> gExportNames;

/*------------------------------------------------------------------*/

static TObject* FollowPath(TObject* container, char* path)
{
  while (1)
    {
      if (0)
        printf("Follow path [%s] in container %p\n", path, container);

      while (*path == '/')
        path++;

      char* s = strchr(path,'/');

      if (s)
        *s = 0;

      TObject *obj = NULL;

      if (container->InheritsFrom(TDirectory::Class()))
        obj = ((TDirectory*)container)->FindObject(path);
      else if (container->InheritsFrom(TFolder::Class()))
        obj = ((TFolder*)container)->FindObject(path);
      else if (container->InheritsFrom(TCollection::Class()))
        obj = ((TCollection*)container)->FindObject(path);
      else
        {
          printf("ERROR: Container \'%s\' of type %s is not a TDirectory, TFolder or TCollection\n", container->GetName(), container->IsA()->GetName());
          return NULL;
        }

      if (!s)
        return obj;

      if (!obj)
        return NULL;

      container = obj;

      path = s+1;
    }
  /* NOT REACHED */
}

static TObject* FindTopLevelObject(const char* name)
{
   TObject *obj = NULL;
   //gROOT->GetListOfFiles()->Print();
   obj = gROOT->GetListOfFiles()->FindObject(name);
   if (obj)
      return obj;
   obj = gROOT->FindObjectAny(name);
   if (obj)
      return obj;
   return NULL;
}

static TObject* TopLevel(char* path, char**opath)
{
  if (0)
    printf("Extract top level object from [%s]\n", path);

  while (*path == '/')
    path++;

  char* s = strchr(path,'/');
  
  if (s)
    {
      *s = 0;
      *opath = s+1;
    }
  else
    {
      *opath = NULL;
    }

  TObject *obj = NULL;

  for (unsigned int i=0; i<gExports.size(); i++)
    {
      const char* ename = gExports[i].c_str();
      //printf("Compare [%s] and [%s]\n", path, ename);
      if (strcmp(path, ename) == 0)
        {
          const char* xname = gExportNames[ename].c_str();
          obj = FindTopLevelObject(xname);
          //printf("Lookup of [%s] returned %p\n", xname, obj);
          break;
        }
    }

  if (!obj)
    {
      printf("ERROR: Top level object \'%s\' not found in exports list\n", path);
	TH1F* objt=new TH1F("test","test",10,0,100);
      return objt;//NULL; //NEW
    }

  return obj;
}

static TObject* FollowPath(char* path)
{
  if (0)
    printf("Follow path [%s]\n", path);

  char *s;
  TObject *obj = TopLevel(path, &s);

  if (!obj)
    return NULL;

  if (!s)
    return obj;

  return FollowPath(obj, s);
}

/*------------------------------------------------------------------*/

static void ResetObject(TObject* obj)
{
  assert(obj!=NULL);

  if (gVerbose)
    printf("ResetObject object %p name [%s] type [%s]\n", obj, obj->GetName(), obj->IsA()->GetName());

  if (obj->InheritsFrom(TH1::Class()))
    {
      ((TH1*)obj)->Reset();
    }
  else if (obj->InheritsFrom(TDirectory::Class()))
    {
      TDirectory* dir = (TDirectory*)obj;
      TList* objs = dir->GetList();

      TIter next = objs;
      while(1)
        {
          TObject *obj = next();
          if (obj == NULL)
            break;
          ResetObject(obj);
        }
    }
}

/*------------------------------------------------------------------*/

static TKey* MakeKey(TObject* obj, int cycle, TDirectory* dir, const char* name = NULL)
{
  static TDirectory* xfile = NULL;
  if (!xfile)
    xfile = new TFile("/dev/null");

  TClass *xclass = obj->IsA();
  
  if (xclass->InheritsFrom(TDirectory::Class()))
    xclass = TDirectory::Class();
  else if (xclass->InheritsFrom(TFolder::Class()))
    xclass = TDirectory::Class();
  else if (xclass->InheritsFrom(TCollection::Class()))
    xclass = TDirectory::Class();

  if (!name)
    name = obj->GetName();

  //return new TKey(name, obj->GetTitle(), xclass, cycle, dir);
  return new TKey(name, obj->GetTitle(), xclass, 0, xfile);
}
 
/*------------------------------------------------------------------*/

static THREADTYPE root_server_thread(void *arg)
/*
  Serve histograms over TCP/IP socket link
*/
{
   char request[2560];

   TSocket *sock = (TSocket *) arg;
   TMessage message(kMESS_OBJECT);

   do {

      /* close connection if client has disconnected */
      int rd = sock->Recv(request, sizeof(request));
      if (rd <= 0)
        {
          if (gVerbose)
            fprintf(stderr, "TNetDirectory connection from %s closed\n", sock->GetInetAddress().GetHostName());
          sock->Close();
          delete sock;
          return THREADRETURN;
        }

      if (gVerbose)
        printf("Request [%s] from %s\n", request, sock->GetInetAddress().GetHostName());

      if (strcmp(request, "GetListOfKeys") == 0)
        {
          // enumerate top level exported directories

          LockRootGuard lock;
          
          //printf("Top level exported directories are:\n");
          TList* keys = new TList();
          
          for (unsigned int i=0; i<gExports.size(); i++)
            {
              const char* ename = gExports[i].c_str();
              const char* xname = gExportNames[ename].c_str();

              TObject* obj = FindTopLevelObject(xname);

              if (!obj)
                {
                  fprintf(stderr, "GetListOfKeys: Exported name \'%s\' cannot be found!\n", xname);
                  continue;
                }

              TKey* key = MakeKey(obj, 1, gROOT, ename);
              keys->Add(key);
            }
          
          if (gVerbose)
            {
              printf("Sending keys %p\n", keys);
              keys->Print();
            }

          message.Reset(kMESS_OBJECT);
          message.WriteObject(keys);
          delete keys;
          lock.Unlock();
          sock->Send(message);
        }
      else if (strncmp(request, "GetListOfKeys ", 14) == 0)
        {
          LockRootGuard lock;

          char* dirname = request + 14;

          TObject* obj = FollowPath(dirname);

          if (obj && obj->InheritsFrom(TDirectory::Class()))
            {
              TDirectory* dir = (TDirectory*)obj;

              //printf("Directory %p\n", dir);
              //dir->Print();
              
              TList* xkeys = dir->GetListOfKeys();
              TList* keys = xkeys;
              if (!keys)
                keys = new TList();

              //printf("Directory %p keys:\n", dir);
              //keys->Print();
              
              TList* objs = dir->GetList();

              //printf("Directory %p objects:\n", dir);
              //objs->Print();
              
              TIter next = objs;
              while(1)
                {
                  TObject *obj = next();

                  //printf("object %p\n", obj);

                  if (obj == NULL)
                    break;
                  
                  const char* name      = obj->GetName();
                  
                  if (!keys->FindObject(name))
                    {
                      TKey* key = MakeKey(obj, 1, dir);
                      keys->Add(key);
                    }
                }

              //printf("Sending keys %p\n", keys);
              //keys->Print();

              message.Reset(kMESS_OBJECT);
              message.WriteObject(keys);
              if (keys != xkeys)
                delete keys;
            }
          else if (obj && obj->InheritsFrom(TFolder::Class()))
            {
              TFolder* folder = (TFolder*)obj;

              //printf("Folder %p\n", folder);
              //folder->Print();

              TIterator *iterator = folder->GetListOfFolders()->MakeIterator();

              TList* keys = new TList();

              while (1)
                {
                  TNamed *obj = (TNamed*)iterator->Next();
                  if (obj == NULL)
                    break;
      
                  const char* name      = obj->GetName();
                  
                  if (!keys->FindObject(name))
                    {
                      TKey* key = MakeKey(obj, 1, gROOT);
                      keys->Add(key);
                    }
                }
              
              delete iterator;

              if (gVerbose)
                {
                  printf("Sending keys %p\n", keys);
                  keys->Print();
                }

              message.Reset(kMESS_OBJECT);
              message.WriteObject(keys);
              delete keys;
            }
          else if (obj && obj->InheritsFrom(TCollection::Class()))
            {
              TCollection* collection = (TCollection*)obj;

              //printf("Collection %p\n", collection);
              //collection->Print();

              TIterator *iterator = collection->MakeIterator();

              TList* keys = new TList();

              while (1)
                {
                  TNamed *obj = (TNamed*)iterator->Next();
                  if (obj == NULL)
                    break;
      
                  const char* name      = obj->GetName();
                  
                  if (!keys->FindObject(name))
                    {
                      TKey* key = MakeKey(obj, 1, gROOT);
                      keys->Add(key);
                    }
                }
              
              delete iterator;

              if (gVerbose)
                {
                  printf("Sending keys %p\n", keys);
                  keys->Print();
                }

              message.Reset(kMESS_OBJECT);
              message.WriteObject(keys);
              delete keys;
            }
          else if (obj)
            {
              fprintf(stderr, "netDirectoryServer: ERROR: obj %p name %s, type %s is not a directory!\n", obj, obj->GetName(), obj->IsA()->GetName());
              TObjString s("Not a directory");
              message.Reset(kMESS_OBJECT);
              message.WriteObject(&s);
            }
          else
            {
              fprintf(stderr, "netDirectoryServer: ERROR: obj %p not found\n", obj);
              TObjString s("Not found");
              message.Reset(kMESS_OBJECT);
              message.WriteObject(&s);
            }
              
          lock.Unlock();
          sock->Send(message);
        }
      else if (strncmp(request, "FindObjectByName ", 17) == 0)
        {
          LockRootGuard lock;

          char* top  = request + 17;

          char *s;
          TObject *obj = TopLevel(top, &s);
		TObject *objt = obj; //NEW

          //printf("toplevel found %p for \'%s\' remaining \'%s\'\n", obj, top, s);

          if (obj && !s)
            {
              // they requested a top-level object. Give out a fake name

              char str[256];
              sprintf(str, "TDirectory %s", obj->GetName());

              for (unsigned int i=0; i<gExports.size(); i++)
                {
                  const char* ename = gExports[i].c_str();
                  const char* xname = gExportNames[ename].c_str();

                  if (strcmp(xname, obj->GetName()) == 0)
                    {
                      sprintf(str, "TDirectory %s", ename);
                      break;
                    }
                }

              obj = new TObjString(str); // FIXME: memory leak!
            }
          else if (obj)
            {
              obj = FollowPath(obj, s);
            }

          if (obj && obj->InheritsFrom(TDirectory::Class()))
            {
              char str[256];
              sprintf(str, "TDirectory %s", obj->GetName());
              obj = new TObjString(str);
            }
          
          if (obj && obj->InheritsFrom(TFolder::Class()))
            {
              char str[256];
              sprintf(str, "TDirectory %s", obj->GetName());
              obj = new TObjString(str);
            }
          
          if (obj && obj->InheritsFrom(TCollection::Class()))
            {
              char str[256];
              sprintf(str, "TDirectory %s", obj->GetName());
              obj = new TObjString(str);
            }
          
          if (gVerbose)
            {
              if (obj)
                printf("Sending object %p name \'%s\' class \'%s\'\n", obj, obj->GetName(), obj->IsA()->GetName());
              else
                printf("Sending object %p\n", obj);
              //obj->Print();
            }

          message.Reset(kMESS_OBJECT);
          message.WriteObject(obj);
          lock.Unlock();
          sock->Send(message);
        }
      else if (strncmp(request, "ResetTH1 ", 9) == 0)
        {
          LockRootGuard lock;
          
          char* path = request + 9;

          if (strlen(path) > 1)
            {
              TObject *obj = FollowPath(path);

              if (obj)
                ResetObject(obj);
            }
          else
            {
              for (unsigned int i=0; i<gExports.size(); i++)
                {
                  const char* ename = gExports[i].c_str();
                  const char* xname = gExportNames[ename].c_str();

                  TObject* obj = FindTopLevelObject(xname);

                  if (!obj)
                    {
                      fprintf(stderr, "ResetTH1: Exported name \'%s\' cannot be found!\n", xname);
                      continue;
                    }

                  ResetObject(obj);
                }
            }
          
          TObjString s("Success");

          message.Reset(kMESS_OBJECT);
          message.WriteObject(&s);
          lock.Unlock();
          sock->Send(message);
        }
      else
        {
          fprintf(stderr, "netDirectoryServer: Received unknown request \"%s\"\n", request);
          LockRootGuard lock;
          TObjString s=request;//("Unknown request"); //NEW
          message.Reset(kMESS_OBJECT);
          message.WriteObject(&s);
          lock.Unlock();
          sock->Send(message);
        }
   } while (1);

   return THREADRETURN;
}

/*------------------------------------------------------------------*/

static THREADTYPE socket_listener(void *arg)
{
  // Server loop listening for incoming network connections on specified port.
  // Starts a searver_thread for each connection.

  int port = *(int *) arg;
  
  fprintf(stderr, "NetDirectory server listening on port %d...\n", port);
  TServerSocket *lsock = new TServerSocket(port, kTRUE);
  
  while (1)
    {
      TSocket *sock = lsock->Accept();
      
      if (sock==NULL)
        {
          printf("TNetDirectory server accept() error\n");
          break;
        }
      
      if (gVerbose)
        fprintf(stderr, "TNetDirectory connection from %s\n", sock->GetInetAddress().GetHostName());
      
#if 1
      TThread *thread = new TThread("NetDirectoryServer", root_server_thread, sock);
      thread->Run();
#else
      LPDWORD lpThreadId = 0;
      CloseHandle(CreateThread(NULL, 1024, &root_server_thread, sock, 0, lpThreadId));
#endif
    }
  
  return THREADRETURN;
}

/*------------------------------------------------------------------*/

void VerboseNetDirectoryServer(bool verbose)
{
  gVerbose = verbose;
  //gDebugLockRoot = verbose;
}

/*------------------------------------------------------------------*/

static bool gAlreadyRunning = false;

void NetDirectoryExport(TDirectory* dir, const char* exportName)
{
  if (gVerbose)
    printf("Export TDirectory %p named [%s] of type [%s] as [%s]\n", dir, dir->GetName(), dir->IsA()->GetName(), exportName);

  bool found = false;
  for (unsigned int i=0; i<gExports.size(); i++)
    {
      const char* ename = gExports[i].c_str();
      if (strcmp(ename, exportName) == 0)
        found = true;
    }

  if (!found)
    gExports.push_back(exportName);
  gExportNames[exportName] = dir->GetName();
}

void NetDirectoryExport(TFolder* folder, const char* exportName)
{
  if (gVerbose)
    printf("Export TFolder %p named [%s] of type [%s] as [%s]\n", folder, folder->GetName(), folder->IsA()->GetName(), exportName);

  bool found = false;
  for (unsigned int i=0; i<gExports.size(); i++)
    {
      const char* ename = gExports[i].c_str();
      if (strcmp(ename, exportName) == 0)
        found = true;
    }

  if (!found)
    gExports.push_back(exportName);
  gExportNames[exportName] = folder->GetName();
}

void NetDirectoryExport(TCollection* collection, const char* exportName)
{
  if (gVerbose)
    printf("Export TCollection %p named [%s] of type [%s] as [%s]\n", collection, collection->GetName(), collection->IsA()->GetName(), exportName);

  bool found = false;
  for (unsigned int i=0; i<gExports.size(); i++)
    {
      const char* ename = gExports[i].c_str();
      if (strcmp(ename, exportName) == 0)
        found = true;
    }

  if (!found)
    gExports.push_back(exportName);
  gExportNames[exportName] = collection->GetName();
}

void StartNetDirectoryServer(int port, TDirectory* dir)
{
  if (dir)
    NetDirectoryExport(dir, dir->GetName());

  if (gAlreadyRunning)
    return;

  if (port==0)
    return;

  gAlreadyRunning = true;

  StartLockRootTimer();

  static int pport = port;
#if 1
  TThread *thread = new TThread("NetDirectoryServer", socket_listener, &pport);
  thread->Run();
#else
  LPDWORD lpThreadId = 0;
  CloseHandle(CreateThread(NULL, 1024, &root_socket_server, &pport, 0, lpThreadId));
#endif
}

// end
