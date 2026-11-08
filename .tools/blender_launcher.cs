using System;
using System.Diagnostics;

class BlenderLauncher
{
    const string Real = @"E:\steam\steamapps\common\Blender\blender.exe";

    static int Main(string[] args)
    {
        string cmd = Environment.CommandLine;
        string rest = "";
        int firstQuote = cmd.IndexOf('"', 0);
        if (firstQuote == 0)
        {
            int secondQuote = cmd.IndexOf('"', 1);
            if (secondQuote > 0) rest = cmd.Substring(secondQuote + 1);
        }
        else
        {
            int sp = cmd.IndexOf(' ');
            if (sp > 0) rest = cmd.Substring(sp + 1);
        }

        var psi = new ProcessStartInfo(Real);
        psi.Arguments = rest;
        psi.UseShellExecute = false;
        using (var p = Process.Start(psi))
        {
            p.WaitForExit();
            return p.ExitCode;
        }
    }
}
