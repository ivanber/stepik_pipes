#include <iostream>
#include <fcntl.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
//------------------------------------------------------------------------------
class Cmd {
public:
  Cmd(std::string const &aCmd, std::vector< std::string > const &aArgs)
    : fCmd(aCmd), fArgs(aArgs) {}

  inline int pipedExec() const {
    int ret = -1;

    int fds[2];
    if (pipe(fds) == 0) {
      ret = fork();
      if (0 == ret) {
        close(STDIN_FILENO);
        close(fds[1]);
        dup2(fds[0], STDIN_FILENO);
        close(fds[0]);
      } else {
        close(STDOUT_FILENO);
        close(fds[0]);
        dup2(fds[1], STDOUT_FILENO);
        close(fds[1]);

        exec();
      }
    }

    return ret;
  }

  inline void exec() const {
    auto const argsSize = fArgs.size() + 2;
    char *args[argsSize];
    std::fill(args, args + argsSize, (char *)0);
    size_t i = 0;
    args[i++] = const_cast< char * >( fCmd.c_str() );
    for (auto const &arg  : fArgs) {
      args[i++] =  const_cast< char *>( arg.c_str() );
    }

    execvp(args[0], args);
  }

  std::string cmd() const { return fCmd; }

  std::vector< std::string > args() const { return fArgs; }
private:
  std::string fCmd;
  std::vector< std::string > fArgs;
};
//------------------------------------------------------------------------------
static std::string const outFileName = "/home/box/result.out";
//------------------------------------------------------------------------------
// trim from end of string (right)
inline std::string& rtrim(std::string& s)
{
    s.erase(s.find_last_not_of(' ') + 1);
    return s;
}
//------------------------------------------------------------------------------
// trim from beginning of string (left)
inline std::string& ltrim(std::string& s)
{
    s.erase(0, s.find_first_not_of(' '));
    return s;
}
//------------------------------------------------------------------------------
// trim from both ends of string (left & right)
inline std::string& trim(std::string& s)
{
    return ltrim(rtrim(s));
}
//------------------------------------------------------------------------------
int main() {
  std::string pipeLine;
  if ( std::getline(std::cin, pipeLine) ) {
    std::vector< Cmd > cmds;

    std::istringstream pipeLineStream(pipeLine);
    std::string cmdLine;
    while ( std::getline(pipeLineStream, cmdLine, '|') ) {
      trim(cmdLine);

      std::istringstream cmdLineStream(cmdLine);

      std::string cmd;
      std::getline(cmdLineStream, cmd, ' ');

      std::vector< std::string > args;
      std::string arg;
      while (std::getline(cmdLineStream, arg, ' ') ) {
        args.push_back(arg);
      }

      cmds.emplace_back(cmd, args);
    }


    for (size_t i = 0, n = cmds.size(); i < n; ++i) {
      Cmd &cmd = cmds[i];
      std::cout << "do command " << cmd.cmd() << std::endl;

      if (n - i == 1) {
        int outFd = open(outFileName.c_str(),
          O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);
        close(STDOUT_FILENO);
        dup2(outFd, STDOUT_FILENO);
        close(outFd);

        cmd.exec();
      } else {
        if ( cmd.pipedExec() ) {
          break;
        }
      }
    }
  }

  return 0;
}
