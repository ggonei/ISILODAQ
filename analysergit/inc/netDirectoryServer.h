//
// netDirectoryServer.h
//
// $Id$
//

class TDirectory;
class TFolder;
class TCollection;

void VerboseNetDirectoryServer(bool verbose);
void StartNetDirectoryServer(int port, TDirectory* dir);
void NetDirectoryExport(TDirectory* dir, const char* exportName);
void NetDirectoryExport(TFolder* folder, const char* exportName);
void NetDirectoryExport(TCollection* collection, const char* exportName);

// end
