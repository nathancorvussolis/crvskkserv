
-------------------------------------------------------------------------------

  crvskkserv 2.5.1

-------------------------------------------------------------------------------

SKK辞書サーバです。

適当な場所に置いて実行して下さい。

タスクトレイのアイコンから設定ダイアログを開き、ポートと辞書を設定して下さい。
辞書の最大数は64個です。

辞書は、SKK辞書ファイル、SKK辞書サーバ、Google CGI API for Japanese Input に
対応しています。
https://www.google.co.jp/ime/cgiapi.html

改行LFでEUC-JIS-2004やEUC-JP、UTF-8(BOMなし)等ASCIIと互換性のある文字コードの
SKK辞書ファイルが使用できます。

SKK辞書の文字コード変換は行っていないので、クライアント側のSKK辞書サーバ通信に
使用する文字コードとSKK辞書ファイルの文字コードを揃える必要があります。

SKK辞書サーバ通信は、IPv4、IPv6に対応しています。

Google CGI APIの「見出し語の検索除外条件」設定は、おそらく送りあり(と思われる)
見出し語を除外する正規表現がデフォルトとなっています。
Visual C++ 2019 の正規表現で、文法はECMAScriptを使用しています。
https://docs.microsoft.com/en-us/cpp/standard-library/regular-expressions-cpp?view=msvc-160

Google CGI APIの設定でクライアントの文字コードをEUC-JIS-2004にしたとき、
UTF-8からEUC-JIS-2004に変換できない文字を含む候補はEUC-JPにフォールバックされます。
EUC-JPにも変換できないときは候補から除外されます。


プロトコル

  "0"

    切断
    応答なし

  "1<見出し語> "

    辞書検索
    応答 : 検索結果 "1/<候補 1>/<候補 2>/ … /<候補 n>\n"
      または 検索結果なし "4\n"

  "2"

    バージョン番号取得
    応答 : "crvskkserv/<major>.<minor>.<build> "
      (本家skkservでは、"<major>.<minor>.<revision> ")

  "3"

    ホスト名取得
    応答 : "<hostname>/<ip address>:<port>/ "
      (本家skkservでは、"<hostname>:<ip address>: " または
        "<hostname>:<ip address 1>:<ip address 2>: … :<ip address n>: ")

  "4<見出し語> "

    サーバーコンプリーション
    応答 : 検索結果なし "4\n"

  その他

    応答なし


-------------------------------------------------------------------------------

  LICENSE

-------------------------------------------------------------------------------

crvskkserv

The MIT License

Copyright (C) 2012-2021 SASAKI Nobuyuki

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

-------------------------------------------------------------------------------

EUC-JIS-2004 (JIS X 0213:2004 Appendix 3) vs Unicode mapping table

Date: 3 May 2009
License:
 Copyright (C) 2001 earthian@tama.or.jp, All Rights Reserved.
 Copyright (C) 2001 I'O, All Rights Reserved.
 Copyright (C) 2006, 2009 Project X0213, All Rights Reserved.
 You can use, modify, distribute this table freely.

-------------------------------------------------------------------------------

JIS X 0208 (1990) to Unicode
© 2015 Unicode®, Inc.

JIS X 0212 (1990) to Unicode
© 2015 Unicode®, Inc.

COPYRIGHT AND PERMISSION NOTICE

Copyright © 1991-2021 Unicode, Inc. All rights reserved.
Distributed under the Terms of Use in https://www.unicode.org/copyright.html.

Permission is hereby granted, free of charge, to any person obtaining
a copy of the Unicode data files and any associated documentation
(the "Data Files") or Unicode software and any associated documentation
(the "Software") to deal in the Data Files or Software
without restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, and/or sell copies of
the Data Files or Software, and to permit persons to whom the Data Files
or Software are furnished to do so, provided that either
(a) this copyright and permission notice appear with all copies
of the Data Files or Software, or
(b) this copyright and permission notice appear in associated
Documentation.

THE DATA FILES AND SOFTWARE ARE PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT OF THIRD PARTY RIGHTS.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS
NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL
DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THE DATA FILES OR SOFTWARE.

Except as contained in this notice, the name of a copyright holder
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in these Data Files or Software without prior
written authorization of the copyright holder.

-------------------------------------------------------------------------------
