using System;
using System.Runtime.InteropServices;

namespace WfpPackaging
{
    // Read the actual Windows trust result rather than a localized error message
    // or PowerShell's UnknownError status. No certificate stores are modified.
    public static class SignatureVerification
    {
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct FileInfo
        {
            public uint Size;
            [MarshalAs(UnmanagedType.LPWStr)] public string Path;
            public IntPtr Handle;
            public IntPtr KnownSubject;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct TrustData
        {
            public uint Size;
            public IntPtr PolicyCallback;
            public IntPtr SipClient;
            public uint UiChoice;
            public uint RevocationChecks;
            public uint UnionChoice;
            public IntPtr File;
            public uint StateAction;
            public IntPtr StateData;
            public IntPtr UrlReference;
            public uint ProviderFlags;
            public uint UiContext;
        }

        [DllImport("wintrust.dll", ExactSpelling = true)]
        private static extern int WinVerifyTrust(IntPtr window, ref Guid action, ref TrustData data);

        public static int Verify(string path)
        {
            var action = new Guid("00aac56b-cd44-11d0-8cc2-00c04fc295ee");
            var file = new FileInfo { Size = (uint)Marshal.SizeOf(typeof(FileInfo)), Path = path };
            var pointer = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(FileInfo)));
            Marshal.StructureToPtr(file, pointer, false);
            var data = new TrustData {
                Size = (uint)Marshal.SizeOf(typeof(TrustData)), UiChoice = 2,
                UnionChoice = 1, File = pointer, StateAction = 1
            };
            try { return WinVerifyTrust(new IntPtr(-1), ref action, ref data); }
            finally
            {
                data.StateAction = 2;
                WinVerifyTrust(new IntPtr(-1), ref action, ref data);
                Marshal.DestroyStructure(pointer, typeof(FileInfo));
                Marshal.FreeHGlobal(pointer);
            }
        }
    }
}
