
pname = '';
fname = 'example_serial_log.txt'; %filename
sname = 'SD Card';  %name to use in the plots

block_size_samp = 128; %audio block size of Tympan
fs_Hz = 44177;  %sample rate of Tympan

%read the file
disp(['Reading ' pname fname]);
fid=fopen([pname fname],'r');
all_txt = textscan(fid,'%s','delimiter',{',' ' ' ':' '='});
all_txt = all_txt{1};
fclose(fid);

%find SD timing entries
I=find(strcmpi(all_txt(1:end-2),'SD') & strcmpi(all_txt(3:end),'us'));
usec = str2num(strvcat(all_txt(I+3)));

%find overrun flags
I=find(strcmpi({all_txt{1:end-1}},'Overrun'));
overrun1 = str2num(strvcat(all_txt(I+2)));
overrun2 = str2num(strvcat(all_txt(I+4)));
memory_usage = str2num(strvcat(all_txt(I+6)));
memory_limit = str2num(strvcat(all_txt(I+8)));
is2_no_memory = str2num(strvcat(all_txt(I+10)));
is_overrun = overrun1 | overrun2 | is2_no_memory | (memory_usage >= memory_limit);

%calculate timing parameters
block_time_sec = block_size_samp/fs_Hz;
block_rate_Hz = 1/block_time_sec;


%% plots
figure;
try;setFigureTallWide;catch;end
ax=[];
c=lines;
all_stats=[];all_stats_usec=[];all_worst_usec=[];
Idata=1;

n = length(usec);
block_ID = [1:n];
t_sec = block_ID/block_rate_Hz;
subplot(2,4,2:3); %top center
semilogy(t_sec,usec/1000,'o-','linewidth',1,'markersize',3);
lt={};lt{end+1}=sname;
xlabel('Time (sec)');
ylabel({'Block Write Time';'(millisec)'});
title('Writing Audio to SD Card from Teensy 3.6');
ylim([1 200]);set(gca,'YTick',[1 10 100],'YTickLabels',{'1' '10' '100'});

if (1)
    %get 1 sec moving average
    lp_sec = 1.0;
    N=round(lp_sec*block_rate_Hz);
    b = 1/N*ones(N,1);a = 1;
    %[b,a]=butter(2,lp_Hz/(block_rate_Hz/2));
    f_usec = filter(b,a,usec-usec(1))+usec(1);
    hold on;
    plot(t_sec,f_usec/1000,'-','linewidth',3);
    lt{end+1}=[num2str(lp_sec) ' sec Ave'];
    xlim([0 t_sec(end)]);
    hold off;
    ylim([0.5 200]);
end
if (1)
    I=find(is_overrun);
    foo_t_sec = t_sec(I);yl=ylim;
    hold on;
    plot(ones(2,1)*(foo_t_sec(:)'),yl(:)*ones(1,length(foo_t_sec)),'r:','linewidth',2);
    lt{end+1}='Overrun';
end

legend(lt);
hold on;plot(xlim,block_time_sec*1000*[1 1],'k--','linewidth',2);
all_stats_usec(Idata,:) = [median(usec) mean(usec) xpercentile(usec,0.99) xpercentile(usec,0.999) max(usec)];
xl = xlim; x = xl(2)+0.05*diff(xl);
yl = ylim; y = yl(2);
text(x,y,{['Median = ' num2str(all_stats_usec(Idata,1)/1000,3) ' ms'];
    ['Mean = ' num2str(all_stats_usec(Idata,2)/1000,3) ' ms'];
    ['99th = ' num2str(all_stats_usec(Idata,3)/1000,3) ' ms'];
    ['99.9th = ' num2str(all_stats_usec(Idata,4)/1000,3) ' ms'];
    ['Max = ' num2str(all_stats_usec(Idata,5)/1000,3) ' ms']}, ...
    'horizontalalignment','left','verticalalignment','top');
all_worst_usec(Idata) = max(usec);
ax(end+1)=gca;

% plot memory usage
subplot(2,2,3);
plot(t_sec,memory_usage,'o-','linewidth',1,'color',c(Idata,:),'markersize',3);
xlabel('Time (sec)');ylabel('F32 Memory Usage');
xlim([0 t_sec(end)]);
hold on;
plot(t_sec,memory_limit,'k--','linewidth',2);
hold off;
yl=ylim; if (yl(2)==max(memory_limit));ylim([yl(1) 1.2*max(memory_limit)]); end;
legend({'Used','Limit'});
title('Reported Use of Audio Memory');
ax(end+1)=gca;

%plot other overrun flags
subplot(2,2,4);
plot(t_sec,[is2_no_memory(:) overrun1(:) overrun2(:) ],'o-','linewidth',1,'markersize',3);
xlabel('Time (sec)');ylabel('Overrun Flag');
ylim([-0.25 2.25]);
xlim([0 t_sec(end)]);
legend({'I2S Input', 'Left Queue','Right Queue'});
hold on;plot(xlim,0.5*[1 1],'k--','linewidth',2);hold off
title('Reported Overrun Flags');
ax(end+1)=gca;

linkaxes(ax,'x');


