#!/bin/perl
   require Term::Screen;

   $scr = new Term::Screen;
   unless ($scr) { die " Something's wrong \n"; }
   $scr->clrscr();
   $scr->at(5,3);
   $scr->puts("this is some stuff");
   $scr->at(10,10)->bold()->puts("hi!")->normal();
      # you can concatenate many calls (not getch)
          $c = $scr->getch();      # doesn't need Enter key 
                if ($scr->key_pressed()) { print "ha you hit a key!"; }




