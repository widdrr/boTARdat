# boTARdat
-----
basic opensource TAR directory access tool
-----
## TODO:
- Choosing libraries
  - [x] Find a hash library (uthash)
  - [x] Find an archive library (libarchive)
- Directory Data Structure
  - [x] Design and implement filesystem metadata
  - [x] Design and implement node struct
  - Implement basic functionality
    - [x] Create node
    - [x] Destroy node
    - [x] Find node by path
    - [x] Add node to parent
    - [x] Remove node from parent
    - [x] Temporary files for write
    - [x] Read from entry
    - [x] Create placeholder node
- Archive IO
  - [x] Create structure from archive
  - [ ] Recreate archive from structure
- FUSE API
  - Technical
    - [x] init
    - [x] destroy
  - Readonly
    - [x] getattr
    - [x] read
    - [x] readdir
    - [ ] lseek
  - Write operations
    - [ ] write
    - [x] mknod
    - [ ] mkdir
    - [x] chmod
    - [x] chow
    - [x] utime
    - [ ] unlink
    - [ ] rmdir
    - [ ] rename
  - Links support
    - [ ] link
    - [ ] symlink
    - [ ] readlink
- Utility
  - [x] Bash script for easier build
- Program
  - [ ] Help prompt
  - [x] Argument parsing (still work to do)
  - [x] Come up with a meaning for boTARdat
