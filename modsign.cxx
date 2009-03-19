/*
  This program signs the given file using the named certificate and private
  key in the given certificate database and places the signature in the named
  output file.

  Copyright (C) 2009 Red Hat Inc.

  This file is part of systemtap, and is free software.  You can
  redistribute it and/or modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "session.h"
#include <iostream>
#include <string>

extern "C" {
#include "nsscommon.h"

#include <nspr.h>
#include <nss.h>
#include <pk11pub.h>
#include <cryptohi.h>

#include <stdio.h>
}

using namespace std;

/* Function: char * password_callback()
 * 
 * Purpose: This function is our custom password handler that is called by
 * NSS when retrieving private certs and keys from the database. Returns a
 * pointer to a string that with a password for the database. Password pointer
 * should point to dynamically allocated memory that will be freed later.
 */
static char *
password_callback (PK11SlotInfo *info, PRBool retry, void *arg)
{
  char *passwd = NULL;

  if (! retry && arg)
    passwd = PORT_Strdup((char *)arg);

  return passwd;
}

/* Obtain the certificate and key database password from the given file.  */
static char *
get_password (const string &fileName)
{
  PRFileDesc *local_file_fd;
  PRFileInfo fileInfo;
  PRInt32 numBytesRead;
  PRStatus prStatus;
  PRInt32 i;
  char *password;

  prStatus = PR_GetFileInfo (fileName.c_str(), &fileInfo);
  if (prStatus != PR_SUCCESS || fileInfo.type != PR_FILE_FILE || fileInfo.size < 0)
    {
      cerr << "Could not obtain information on password file " << fileName << "." << endl;
      nssError ();
      return NULL;
    }

  local_file_fd = PR_Open (fileName.c_str(), PR_RDONLY, 0);
  if (local_file_fd == NULL)
    {
      cerr << "Could not open password file " << fileName << "." << endl;
      nssError ();
      return NULL;
    }
      
  password = (char*)PORT_Alloc (fileInfo.size + 1);
  if (! password)
    {
      cerr << "Unable to allocate " << (fileInfo.size + 1) << " bytes." << endl;
      nssError ();
      return NULL;
    }

  numBytesRead = PR_Read (local_file_fd, password, fileInfo.size);
  if (numBytesRead <= 0)
    {
      cerr << "Error reading password file " << fileName << "." << endl;
      nssError ();
      return 0;
    }

  PR_Close (local_file_fd);

  /* Keep only the first line of data.  */
  for (i = 0; i < numBytesRead; ++i)
    {
      if (password[i] == '\n' || password[i] == '\r' || password[i] == '\0')
	break;
    }
  password[i] = '\0';

  return password;
}

static void
sign_it (const string &inputName, const string &outputName, SECKEYPrivateKey *privKey)
{
  unsigned char buffer[4096];
  PRFileDesc *local_file_fd;
  PRUint32 numBytes;
  SECStatus secStatus;
  SGNContext *sgn;
  SECItem signedData;

  /* Set up the signing context.  */
  sgn = SGN_NewContext (SEC_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION, privKey);
  if (! sgn) 
    {
      cerr << "Could not create signing context." << endl;
      nssError ();
      return;
    }
  secStatus = SGN_Begin (sgn);
  if (secStatus != SECSuccess)
    {
      cerr << "Could not initialize signing context." << endl;
      nssError ();
      return;
    }

  /* Now read the data and add it to the signature.  */
  local_file_fd = PR_Open (inputName.c_str(), PR_RDONLY, 0);
  if (local_file_fd == NULL)
    {
      cerr << "Could not open module file " << inputName << "." << endl;
      nssError ();
      return;
    }

  for (;;)
    {
      numBytes = PR_Read (local_file_fd, buffer, sizeof (buffer));
      if (numBytes == 0)
	break;	/* EOF */

      if (numBytes < 0)
	{
	  cerr << "Error reading module file " << inputName << "." << endl;
	  nssError ();
	  return;
	}

      /* Add the data to the signature.  */
      secStatus = SGN_Update (sgn, buffer, numBytes);
      if (secStatus != SECSuccess)
	{
	  cerr << "Error while signing module file " << inputName << "." << endl;
	  nssError ();
	  return;
	}
    }

  PR_Close (local_file_fd);

  /* Complete the signature.  */
  secStatus = SGN_End (sgn, & signedData);
  if (secStatus != SECSuccess)
    {
      cerr << "Could not complete signature of module file " << inputName << "." << endl;
      nssError ();
      return;
    }

  SGN_DestroyContext (sgn, PR_TRUE);

  /* Now write the signed data to the output file.  */
  local_file_fd = PR_Open (outputName.c_str(), PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE,
			   PR_IRUSR | PR_IWUSR | PR_IRGRP | PR_IWGRP | PR_IROTH);
  if (local_file_fd == NULL)
    {
      cerr << "Could not open signature file " << outputName << "." << endl;
      nssError ();
      return;
    }

  numBytes = PR_Write (local_file_fd, signedData.data, signedData.len);
  if (numBytes < 0 || numBytes != signedData.len)
    {
      cerr << "Error writing to signature file " << outputName << "." << endl;
      nssError ();
      return;
    }

  PR_Close (local_file_fd);
}

void
sign_module (systemtap_session& s)
{
  const char *nickName = "stap-server";
  char *password;
  CERTCertificate *cert;
  SECKEYPrivateKey *privKey;
  SECStatus secStatus;

  password = get_password (s.cert_db_path + "/pw");
  if (! password)
    {
      cerr << "Unable to obtain certificate database password." << endl;
      return;
    }

  /* Call the NSPR initialization routines. */
  PR_Init (PR_SYSTEM_THREAD, PR_PRIORITY_NORMAL, 1);

  /* Set the cert database password callback. */
  PK11_SetPasswordFunc (password_callback);

	/* Initialize NSS. */
  secStatus = NSS_Init (s.cert_db_path.c_str());
  if (secStatus != SECSuccess)
    {
      cerr << "Unable to initialize nss library." << endl;
      nssError ();
      return;
    }

  /* Get own certificate and private key. */
  cert = PK11_FindCertFromNickname (nickName, password);
  if (cert == NULL)
    {
      cerr << "Unable to find certificate with nickname " << nickName
	   << " in " << s.cert_db_path << "." << endl;
      nssError ();
      return;
    }

  privKey = PK11_FindKeyByAnyCert (cert, password);
  if (privKey == NULL)
    {
      cerr << "Unable to obtain private key from the certificate with nickname " << nickName
	   << " in " << s.cert_db_path << "." << endl;
      nssError ();
      return;
    }

  /* Sign the file. */
  sign_it (s.tmpdir + "/" + s.module_name + ".ko", s.tmpdir + "/" + s.module_name + ".ko.sgn", privKey);

  /* Shutdown NSS and exit NSPR gracefully. */
  nssCleanup ();
}