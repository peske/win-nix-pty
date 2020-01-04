using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using PtyClr;

namespace Test
{
    class Program
    {
        // ReSharper disable once UnusedParameter.Local
        static void Main(string[] args)
        {
            using (var terminal = new Pty(PtyBuild.Cygwin))
            {
                //SpawnCygwinBash(terminal);
                SpawnGitBash(terminal);
                // ReSharper disable once AssignmentIsFullyDiscarded
                _ = ReadOutputAsync(terminal);
                
                ReadInput(terminal);
            }

            Console.WriteLine("Bye-bye!");
        }

        private static Dictionary<string, string> GetUsualEnvVars() => new Dictionary<string, string>
        {
            {"TERM", "xterm-256color"},
            {"HOME", @"C:\Users\fatdragon"}
        };

        // ReSharper disable once UnusedMember.Local
        private static void SpawnGitBash(Pty terminal)
        {
            var envVars = GetUsualEnvVars();
            envVars["PATH"] = @"C:\Program Files\Git\usr\bin;%PATH%";

            terminal.Spawn(@"C:\Program Files\Git\bin\bash.exe", "-i -l", logLevel: LogLevel.Trace, sysLog: true,
                environmentVariables: envVars);
        }

        // ReSharper disable once UnusedMember.Local
        private static void SpawnCygwinBash(Pty terminal)
        {
            var envVars = GetUsualEnvVars();
            envVars["PATH"] = @"C:\cygwin64\bin;%PATH%";

            terminal.Spawn(@"C:\cygwin64\bin\bash.exe", "-i -l", logLevel: LogLevel.Trace, sysLog: true,
                environmentVariables: envVars);
        }

        private static void ReadInput(Pty terminal)
        {
            var buff = new byte[100];

            while (true)
            {
                int read;

                try
                {
                    read = Console.OpenStandardInput().Read(buff, 0, buff.Length);
                }
                catch (Exception e)
                {
                    Console.Error.WriteLine(e);
                    return;
                }

                if (read < 1)
                    continue;

                var toWrite = buff.Take(read).ToArray();

                try
                {
                    terminal.WriteInput(toWrite);
                }
                catch (Exception e)
                {
                    Console.Error.WriteLine(e);
                    return;
                }
            }
        }

        private static async Task ReadOutputAsync(Pty terminal)
        {
            var buff = new byte[4096];

            while (true)
            {
                int read;

                try
                {
                    read = await terminal.OutputStream.ReadAsync(buff, 0, buff.Length).ConfigureAwait(false);
                }
                catch (Exception e)
                {
                    Console.Error.WriteLine(e);
                    return;
                }

                if (read > 0)
                    Console.OpenStandardOutput(read).Write(buff, 0, read);
            }
        }
    }
}
