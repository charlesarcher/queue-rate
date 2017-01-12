import os,re,types,math
import xlsxwriter
import Queue as Q
from xlsxwriter.utility import xl_rowcol_to_cell
from collections import defaultdict

def nesteddict():
    return defaultdict(list)

benchmarks    = []
sheetidx      = {}
worksheets    = {}
column_charts = {}

wb   = xlsxwriter.Workbook("plot.xlsx")

extensions = ['out'] ;
file_names = [fn for fn in os.listdir(os.curdir) if any([fn.endswith(ext) for ext in extensions])];

for file_name in file_names:
    ws       = wb.add_worksheet(file_name)
    ws.write_number(1, 1, float(0.0))
    for line in open(file_name, 'r'):
        parsed=line.split()
        print parsed
        print parsed[1], parsed[2], parsed[4]
        ws.write_number(0,int(parsed[2]),int(parsed[2]));
        ws.write_number(int(parsed[1]),0,int(parsed[1]));
        ws.write_number(int(parsed[1]),int(parsed[2]), float(parsed[4]))


wb.close()
