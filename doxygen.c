               
/*! \file globaldecls.h
      \brief Place to look for global variables, enums, functions
           and macro definitions
  */

/** \var const int fileSize
      \brief Default size of the file on disk
  */
const int fileSize = 1048576;

/** \def SHIFT(value, length)
      \brief Left shift value by length in bits
  */
#define SHIFT(value, length) ((value) << (length))

/** \fn bool check_for_io_errors(FILE* fp)
      \brief Checks if a file is corrupted or not
      \param fp Pointer to an already opened file
      \warning Not thread safe!
  */
bool check_for_io_errors(FILE* fp);
