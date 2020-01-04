namespace PtyClr
{
    internal enum Command : byte
    {
        Ping = 1,
        GetWinSize = 2,
        SetWinSize = 3,
        GetAttributes = 4,
        SetAttributes = 5
    }
}